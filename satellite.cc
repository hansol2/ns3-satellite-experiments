#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/satellite-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/pcap-file-wrapper.h"

using namespace ns3;

Ptr<PcapFileWrapper> globalPcapFile;

template <typename T>
void GlobalPacketCapture(std::string context, Ptr<const T> packet) {
    globalPcapFile->Write(Simulator::Now(), packet);
}

void SetupGlobalPcapCapture() {
    PcapHelper pcapHelper;
    globalPcapFile = pcapHelper.CreateFile("global_capture.pcap", std::ios::out, PcapHelper::DLT_EN10MB);
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/Tx", MakeCallback(&GlobalPacketCapture<Packet>));
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/Rx", MakeCallback(&GlobalPacketCapture<Packet>));
}

int main(int argc, char *argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);

    // SatHelper로 위성 네트워크 설정
    SatHelper satHelper;
    satHelper.CreatePredefinedScenario(SatHelper::SIMPLE); // 기본 시나리오: UT, Satellite, GW 포함

    // 노드 컨테이너 가져오기
    NodeContainer utNodes = satHelper.UtNodes();
    NodeContainer satNodes = satHelper.SatNodes();
    NodeContainer gwNodes = satHelper.GwNodes();

    // 추가 노드 생성 (endUser1, endUser2)
    NodeContainer endUser1, endUser2;
    endUser1.Create(1);
    endUser2.Create(1);

    // 위치 설정
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(endUser1);
    endUser1.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0.0, 0.0, 0.0));
    mobility.Install(utNodes);
    utNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(500.0, 0.0, 0.0));
    mobility.Install(satNodes);
    satNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0.0, 0.0, 36000000.0));
    mobility.Install(gwNodes);
    gwNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(1000.0, 0.0, 0.0));
    mobility.Install(endUser2);
    endUser2.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(2000.0, 0.0, 0.0));

    // 유선 링크: endUser1 <-> UT
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer devUserUt = csma.Install(NodeContainer(endUser1.Get(0), utNodes.Get(0)));

    // SatHelper가 생성한 UT-Satellite, Satellite-GW 링크의 디바이스 가져오기
    // SatHelper 내부에서 자동으로 설정된 디바이스를 직접 접근
    Ptr<NetDevice> utSatDevice = utNodes.Get(0)->GetDevice(0); // UT의 첫 번째 디바이스
    Ptr<NetDevice> satUtDevice = satNodes.Get(0)->GetDevice(0); // Satellite의 UT 연결 디바이스
    Ptr<NetDevice> satGwDevice = satNodes.Get(0)->GetDevice(1); // Satellite의 GW 연결 디바이스
    Ptr<NetDevice> gwSatDevice = gwNodes.Get(0)->GetDevice(0); // GW의 첫 번째 디바이스

    NetDeviceContainer devUtSat;
    devUtSat.Add(utSatDevice);
    devUtSat.Add(satUtDevice);

    NetDeviceContainer devSatGw;
    devSatGw.Add(satGwDevice);
    devSatGw.Add(gwSatDevice);

    // 유선 링크: GW <-> endUser2
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("0.1ms"));
    NetDeviceContainer devGwUser = p2p.Install(NodeContainer(gwNodes.Get(0), endUser2.Get(0)));

    // 인터넷 스택 설치
    InternetStackHelper internet;
    internet.InstallAll();

    // IP 주소 할당
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifUserUt = ipv4.Assign(devUserUt);
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer ifUtSat = ipv4.Assign(devUtSat);
    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer ifSatGw = ipv4.Assign(devSatGw);
    ipv4.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer ifGwUser = ipv4.Assign(devGwUser);

    // 라우팅 설정
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 애플리케이션 설정
    uint16_t port = 8080;
    Address serverAddress(InetSocketAddress(ifGwUser.GetAddress(1), port));
    PacketSinkHelper tcpServer("ns3::TcpSocketFactory", serverAddress);
    ApplicationContainer serverApps = tcpServer.Install(endUser2.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(20.0));

    OnOffHelper tcpClient("ns3::TcpSocketFactory", serverAddress);
    tcpClient.SetAttribute("DataRate", StringValue("1Mbps"));
    tcpClient.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps = tcpClient.Install(endUser1.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(20.0));

    // 패킷 캡처
    SetupGlobalPcapCapture();

    Simulator::Run();
    Simulator::Destroy();
    globalPcapFile->Close();

    return 0;
}
