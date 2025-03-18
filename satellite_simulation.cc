#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SatelliteNetworkStable");

int main(int argc, char *argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);

    NS_LOG_INFO("Starting Stable Satellite Network Simulation...");

    // 노드 생성
    NodeContainer endUser1, ut, satellite, gw, endUser2;
    endUser1.Create(1);
    ut.Create(1);
    satellite.Create(1);
    gw.Create(1);
    endUser2.Create(1);

    // CSMA 채널 (EndUser1 <-> UT)
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("1ms"));

    NetDeviceContainer devUserUt = csma.Install(NodeContainer(endUser1.Get(0), ut.Get(0)));

    // 위성 링크 (PointToPointHelper)
    PointToPointHelper satelliteLink;
    satelliteLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    satelliteLink.SetChannelAttribute("Delay", StringValue("250ms"));

    NetDeviceContainer devUtSat = satelliteLink.Install(NodeContainer(ut.Get(0), satellite.Get(0)));
    NetDeviceContainer devSatGw = satelliteLink.Install(NodeContainer(satellite.Get(0), gw.Get(0)));

    // GW <-> EndUser2 연결 (P2P 고속 링크, 유일 경로)
    PointToPointHelper gwToUserLink;
    gwToUserLink.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    gwToUserLink.SetChannelAttribute("Delay", StringValue("0.1ms"));

    NetDeviceContainer devGwEndUser2 = gwToUserLink.Install(NodeContainer(gw.Get(0), endUser2.Get(0)));

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
    Ipv4InterfaceContainer ifSatGw = ipv4.Assign
