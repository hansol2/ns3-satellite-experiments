#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/satellite-module.h" // SNS-3 전용 모듈
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
    globalPcapFile = pcapHelper.CreateFile("global_capture.pcap", std::ios::out, PcapHelper::DLT_EN10MB); // Ethernet 호환
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/Tx", MakeCallback(&GlobalPacketCapture<Packet>));
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/Rx", MakeCallback(&GlobalPacketCapture<Packet>));
}

int main(int argc, char *argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);

    // 노드 생성
    NodeContainer endUser1, ut, satellite, gw, endUser2;
    endUser1.Create(1);
    ut.Create(1);
    satellite.Create(1);
    gw.Create(1);
    endUser2.Create(1);

    // SNS-3 위성 헬퍼 설정
    SatHelper satHelper;
    satHelper.SetBeamNetworkType(SatHelper::GEO); // 정지궤도 위성
    satHelper.CreatePredefinedScenario(SatHelper::SIMPLE); // 기본 시나리오 재사용 후 수정

    // 위치 설정
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(endUser1);
    endUser1.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0.0, 0.0, 0.0));
    mobility.Install(ut);
    ut.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(500.0, 0.0, 0.0));
    mobility.Install(satellite);
    satellite.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0.0, 0.0, 36000000.0)); // GEO 36,000 km
    mobility.Install(gw);
    gw.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(1000.0, 0.0, 0.0));
    mobility.Install(endUser2);
    endUser2.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(2000.0, 0.0, 0.0));

    // 유선 링크: endUser1 <-> ut
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer devUserUt = csma.Install(NodeContainer(endUser1.Get(0), ut.Get(0)));

    // 위성 링크: ut <-> satellite <-> gw (SNS-3 모듈 사용)
    NetDeviceContainer devUtSat = satHelper.InstallUserLink(ut, satellite);
    NetDeviceContainer devSatGw = satHelper.InstallGwLink(satellite, gw);

    // 유선 링크: gw <-> endUser2
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("0.1ms"));
    NetDeviceContainer devGwUser = p2p.Install(NodeContainer(gw.Get(0), endUser2.Get(0)));

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
