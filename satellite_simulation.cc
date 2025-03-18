#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/satellite-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SatelliteNetwork");

int main(int argc, char *argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);

    NS_LOG_INFO("Starting Satellite Network Simulation...");

    // 1️⃣ 노드 생성 (EndUser1 - UT - Satellite - GW - EndUser2)
    NodeContainer endUser1, ut, satellite, gw, endUser2;
    endUser1.Create(1);
    ut.Create(1);
    satellite.Create(1);
    gw.Create(1);
    endUser2.Create(1);

    // 2️⃣ CSMA 채널 설정 (EndUser ↔ UT, GW ↔ EndUser)
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("1ms"));

    NetDeviceContainer devUserUt = csma.Install(NodeContainer(endUser1.Get(0), ut.Get(0)));
    NetDeviceContainer devGwUser = csma.Install(NodeContainer(gw.Get(0), endUser2.Get(0)));

    // 3️⃣ SAT 채널 설정 (UT ↔ Satellite, Satellite ↔ GW) ✅ SetAttribute() 사용
    Ptr<SatChannel> satChannel = CreateObject<SatChannel>(); // ✅ 위성 채널 객체 직접 생성

    SatNetDeviceHelper satNetDeviceHelper;
    satNetDeviceHelper.SetAttribute("Delay", TimeValue(MilliSeconds(250)));  // ✅ 위성 전송 지연 설정
    satNetDeviceHelper.SetAttribute("DataRate", DataRateValue(DataRate("10Mbps")));  // ✅ 위성 링크 속도 설정

    NetDeviceContainer devUtSat = satNetDeviceHelper.Install(ut.Get(0), satellite.Get(0), satChannel);
    NetDeviceContainer devSatGw = satNetDeviceHelper.Install(satellite.Get(0), gw.Get(0), satChannel);

    // 4️⃣ Ideal 채널 설정 (GW ↔ EndUser2)
    PointToPointHelper idealLink;
    idealLink.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    idealLink.SetChannelAttribute("Delay", StringValue("0.1ms"));

    NetDeviceContainer devIdeal = idealLink.Install(NodeContainer(gw.Get(0), endUser2.Get(0)));

    // 5️⃣ 인터넷 스택 설치
    InternetStackHelper internet;
    internet.Install(endUser1);
    internet.Install(ut);
    internet.Install(satellite);
    internet.Install(gw);
    internet.Install(endUser2);

    // 6️⃣ IP 주소 할당
    Ipv4AddressHelper ipv4;

    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifUserUt = ipv4.Assign(devUserUt);

    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer ifUtSat = ipv4.Assign(devUtSat);

    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer ifSatGw = ipv4.Assign(devSatGw);

    ipv4.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer ifGwUser = ipv4.Assign(devGwUser);

    ipv4.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer ifIdeal = ipv4.Assign(devIdeal);

    // 7️⃣ TCP 서버 설정 (EndUser2에서 실행)
    uint16_t port = 8080;
    Address serverAddress(InetSocketAddress(ifIdeal.GetAddress(1), port)); // ✅ 수정 완료
    PacketSinkHelper tcpServer("ns3::TcpSocketFactory", serverAddress);
    ApplicationContainer serverApps = tcpServer.Install(endUser2.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(20.0));

    // 8️⃣ TCP 클라이언트 설정 (EndUser1에서 실행)
    OnOffHelper tcpClient("ns3::TcpSocketFactory", serverAddress);
    tcpClient.SetAttribute("DataRate", StringValue("1Mbps"));
    tcpClient.SetAttribute("PacketSize", UintegerValue(1024));
    tcpClient.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    tcpClient.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer clientApps = tcpClient.Install(endUser1.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(20.0));

    // 9️⃣ 패킷 캡처 활성화 (PCAP 파일 저장)
    csma.EnablePcapAll("satellite-network");
    satNetDeviceHelper.EnablePcapAll("satellite-network");  // ✅ SAT 채널 패킷 캡처로 수정 완료
    idealLink.EnablePcapAll("satellite-network");

    NS_LOG_INFO("Running Simulation...");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Simulation completed.");

    return 0;
}

