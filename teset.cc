#include "sns3/satellite-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/pcap-file-wrapper.h"

using namespace sns3;
using namespace ns3;

int main(int argc, char *argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    // 1. GEO 위성 생성
    SatelliteContainer geoSat = SatelliteHelper::CreateConstellation("GEO", 1, 1, 35786000); // 1개 GEO 위성

    // 2. 지상국 2개 생성
    GroundStationContainer groundStations;
    groundStations.Create(2);

    // 3. 궤도 전파 적용
    OrbitPropagatorHelper orbitHelper;
    orbitHelper.Install(geoSat);

    // 4. 무선 링크 설정 (지상국 ↔ 위성)
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel", "Frequency", DoubleValue(14e9)); // 14GHz 주파수
    wifiChannel.AddPropagationLoss("ns3::ConstantLossModel", "Loss", DoubleValue(2));
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");

    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());
    wifiPhy.Set("TxPowerStart", DoubleValue(40));
    wifiPhy.Set("TxPowerEnd", DoubleValue(40));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer gs1ToSat = wifi.Install(wifiPhy, wifiMac, NodeContainer(groundStations.Get(0), geoSat.Get(0)));
    NetDeviceContainer gs2ToSat = wifi.Install(wifiPhy, wifiMac, NodeContainer(groundStations.Get(1), geoSat.Get(0)));

    // 5. 인터넷 스택 설치
    InternetStackHelper internet;
    internet.Install(groundStations);
    internet.Install(geoSat);

    // 6. IP 주소 부여
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaceGs1Sat = ipv4.Assign(gs1ToSat);

    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaceGs2Sat = ipv4.Assign(gs2ToSat);

    // 7. TCP 서버 (지상국2)
    uint16_t port = 50000;
    Address serverAddress(InetSocketAddress(ifaceGs2Sat.GetAddress(0), port));
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", serverAddress);
    ApplicationContainer serverApp = sinkHelper.Install(groundStations.Get(1));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(30.0));

    // 8. TCP 클라이언트 (지상국1)
    OnOffHelper client("ns3::TcpSocketFactory", serverAddress);
    client.SetAttribute("DataRate", StringValue("1Mbps"));
    client.SetAttribute("PacketSize", UintegerValue(1024));
    client.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    client.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer clientApp = client.Install(groundStations.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(30.0));

    // 9. PCAP 캡처
    wifiPhy.EnablePcap("geo_tcp_wireless_traffic", gs1ToSat.Get(0));

    // 10. 시뮬레이션 실행
    Simulator::Stop(Seconds(31.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
