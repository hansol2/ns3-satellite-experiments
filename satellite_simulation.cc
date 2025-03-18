#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SatelliteNetworkDebug");

int main(int argc, char *argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);

    NS_LOG_INFO("Starting Satellite Network Simulation (Debug Version)...");

    // 노드 생성
    NodeContainer endUser1, ut, satellite, gw, endUser2;
    endUser1.Create(1);
    ut.Create(1);
    satellite.Create(1);
    gw.Create(1);
    endUser2.Create(1);

    // CSMA 채널 설정
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("1ms"));

    NetDeviceContainer devUserUt = csma.Install(NodeContainer(endUser1.Get(0), ut.Get(0)));
    NetDeviceContainer devGwUser = csma.Install(NodeContainer(gw.Get(0), endUser2.Get(0)));

    // 위성 링크 (PointToPointHelper)
    PointToPointHelper satelliteLink;
    satelliteLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    satelliteLink.SetChannelAttribute("Delay", StringValue("250ms"));

    NetDeviceContainer devUtSat = satelliteLink.Install(NodeContainer(ut.Get(0), satellite.Get(0)));
    NetDeviceContainer devSatGw = satelliteLink.Install(NodeContainer(satellite.Get(0), gw.Get(0)));

    // GW <-> EndUser2 링크
    PointToPointHelper idealLink;
    idealLink.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    idealLink.SetChannelAttribute("Delay", StringValue("0.1ms"));

    NetDeviceContainer devIdeal = idealLink.Install(NodeContainer(gw.Get(0), endUser2.Get(0)));

    // 인터넷 스택 설치
    InternetStackHelper internet;
    internet.Install(endUser1);
    internet.Install(ut);
    internet.Install(satellite);
    internet.Install(gw);
    internet.Install(endUser2);

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

    ipv4.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer ifIdeal = ipv4.Assign(devIdeal);

    // TCP 서버 (EndUser2)
    uint16_t port = 8080;
    Address serverAddress(InetSocketAddress(ifGwUser.GetAddress(1), port)); // 디버그: ifGwUser 주소 사용
    PacketSinkHelper tcpServer("ns3::TcpSocketFactory", serverAddress);
    ApplicationContainer serverApps = tcpServer.Install(endUser2.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(30.0)); // 더 길게 실행

    // TCP 클라이언트 (EndUser1)
    OnOffHelper tcpClient("ns3::TcpSocketFactory", serverAddress);
    tcpClient.SetAttribute("DataRate", StringValue("1Mbps"));
    tcpClient.SetAttribute("PacketSize", UintegerValue(1024));
    tcpClient.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    tcpClient.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer clientApps = tcpClient.Install(endUser1.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(30.0));

    // 패킷 캡처 활성화
    csma.EnablePcapAll("satellite-network-csma-debug");
    satelliteLink.EnablePcapAll("satellite-network-satlink-debug");
    idealLink.EnablePcapAll("satellite-network-p2p-debug");

    Simulator::Stop(Seconds(30.0)); // 시뮬레이션 명시적 종료 시간

    NS_LOG_INFO("Running Simulation (Debug Version)...");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Simulation completed (Debug).");

    return 0;
}
