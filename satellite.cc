#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/satellite-point-to-point-isl-helper.h"
#include "ns3/pcap-file-wrapper.h"

using namespace ns3;

Ptr<PcapFileWrapper> globalPcapFile;

template <typename T>
void GlobalPacketCapture(std::string context, Ptr<const T> packet) {
    globalPcapFile->Write(Simulator::Now(), packet);
}

void SetupGlobalPcapCapture() {
    PcapHelper pcapHelper;
    globalPcapFile = pcapHelper.CreateFile("global_capture.pcap", std::ios::out, PcapHelper::DLT_RAW);

    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/Tx", MakeCallback(&GlobalPacketCapture<Packet>));
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/Rx", MakeCallback(&GlobalPacketCapture<Packet>));
}

int main(int argc, char *argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);

    NS_LOG_INFO("Starting Satellite Network Simulation with ISL Helper and Global PCAP Capture...");

    NodeContainer endUser1, ut, satellite, gw, endUser2;
    endUser1.Create(1);
    ut.Create(1);
    satellite.Create(1);
    gw.Create(1);
    endUser2.Create(1);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("1ms"));

    NetDeviceContainer devUserUt = csma.Install(NodeContainer(endUser1.Get(0), ut.Get(0)));
    NetDeviceContainer devGwUser = csma.Install(NodeContainer(gw.Get(0), endUser2.Get(0)));

    SatellitePointToPointIslHelper satLink;
    satLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    satLink.SetChannelAttribute("MaxPropagationDelay", TimeValue(Seconds(0.5)));

    NetDeviceContainer devUtSat = satLink.Install(ut.Get(0), satellite.Get(0));
    NetDeviceContainer devSatGw = satLink.Install(satellite.Get(0), gw.Get(0));

    PointToPointHelper idealLink;
    idealLink.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    idealLink.SetChannelAttribute("Delay", StringValue("0.1ms"));

    NetDeviceContainer devIdeal = idealLink.Install(NodeContainer(gw.Get(0), endUser2.Get(0)));

    InternetStackHelper internet;
    internet.Install(endUser1);
    internet.Install(ut);
    internet.Install(satellite);
    internet.Install(gw);
    internet.Install(endUser2);

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

    uint16_t port = 8080;
    Address serverAddress(InetSocketAddress(ifIdeal.GetAddress(1), port));
    PacketSinkHelper tcpServer("ns3::TcpSocketFactory", serverAddress);
    ApplicationContainer serverApps = tcpServer.Install(endUser2.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(20.0));

    OnOffHelper tcpClient("ns3::TcpSocketFactory", serverAddress);
    tcpClient.SetAttribute("DataRate", StringValue("1Mbps"));
    tcpClient.SetAttribute("PacketSize", UintegerValue(1024));
    tcpClient.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    tcpClient.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer clientApps = tcpClient.Install(endUser1.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(20.0));

    SetupGlobalPcapCapture();

    NS_LOG_INFO("Running Simulation with ISL support...");
    Simulator::Run();
    Simulator::Destroy();

    globalPcapFile->Close();

    NS_LOG_INFO("Simulation completed.");
    return 0;
}
