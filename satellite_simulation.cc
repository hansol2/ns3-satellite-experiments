#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main(int argc, char *argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);

    // 1️⃣ 노드 생성
    NodeContainer endUser1, ut, satellite, gw, endUser2;
    endUser1.Create(1);
    ut.Create(1);
    satellite.Create(1);
    gw.Create(1);
    endUser2.Create(1);

    // 2️⃣ EndUser1 <-> UT (CSMA 링크)
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer devUserUt = csma.Install(NodeContainer(endUser1.Get(0), ut.Get(0)));

    // 3️⃣ UT <-> Satellite, Satellite <-> GW (위성 링크)
    PointToPointHelper satLink;
    satLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    satLink.SetChannelAttribute("Delay", StringValue("250ms"));
    NetDeviceContainer devUtSat = satLink.Install(ut.Get(0), satellite.Get(0));
    NetDeviceContainer devSatGw = satLink.Install(satellite.Get(0), gw.Get(0));

    // 4️⃣ GW <-> EndUser2 (고속 링크)
    PointToPointHelper gwToUserLink;
    gwToUserLink.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    gwToUserLink.SetChannelAttribute("Delay", StringValue("0.1ms"));
    NetDeviceContainer devGwUser = gwToUserLink.Install(NodeContainer(gw.Get(0), endUser2.Get(0)));

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

    // ⭐ 할당된 IP 주소 출력해서 점검!
    std::cout << "EndUser2 IP: " << ifGwUser.GetAddress(1) << std::endl;

    // ⭐ 라우팅 테이블 생성 (중요!)
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 7️⃣ TCP 서버 (EndUser2)
    uint16_t port = 8080;
    Address serverAddress(InetSocketAddress(ifGwUser.GetAddress(1), port));
    PacketSinkHelper sink("ns3::TcpSocketFactory", serverAddress);
    ApplicationContainer serverApp = sink.Install(endUser2.Get(0));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(20.0));

    // 8️⃣ TCP 클라이언트 (EndUser1)
    OnOffHelper client("ns3::TcpSocketFactory", serverAddress);
    client.SetAttribute("DataRate", StringValue("1Mbps"));
    client.SetAttribute("PacketSize", UintegerValue(1024));
    client.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    client.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer clientApp = client.Install(endUser1.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(20.0));

    // 9️⃣ 패킷 캡처 활성화
    csma.EnablePcapAll("csma");
    satLink.EnablePcapAll("satlink");
    gwToUserLink.EnablePcapAll("gw-user");

    // 🔟 FlowMonitor 설치 (트래픽 흐름 & 성능 분석용)
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();

    // 1️⃣1️⃣ 시뮬레이션 실행
    Simulator::Stop(Seconds(20.0));
    Simulator::Run();

    // 1️⃣2️⃣ FlowMonitor 결과 저장
    flowMonitor->SerializeToXmlFile("flowmonitor-results.xml", true, true);

    Simulator::Destroy();
    return 0;
}
