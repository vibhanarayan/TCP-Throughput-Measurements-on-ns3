#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/bulk-send-application.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("P1");

int
main (int argc, char *argv[])
{
  SeedManager::SetSeed(1); 

  bool verbose = true;
  uint32_t nSpokes = 8;
  uint32_t maxBytes = 0;
  std::string transport_prot = "TcpWestwood";
  
  CommandLine cmd;
  cmd.AddValue ("nSpokes", "Number spokes on each star", nSpokes);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("transport_prot", "Transport protocol to use: TcpNewReno, "
  "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
  "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus ", transport_prot);
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);

  if (verbose)
  {
    LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
  }

  // Select TCP variant
  if (transport_prot.compare ("TcpNewReno") == 0)
  {
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
  }
else if (transport_prot.compare ("TcpHybla") == 0)
  {
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHybla::GetTypeId ()));
  }
else if (transport_prot.compare ("TcpHighSpeed") == 0)
  {
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHighSpeed::GetTypeId ()));
  }
else if (transport_prot.compare ("TcpVegas") == 0)
  {
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
  }
else if (transport_prot.compare ("TcpScalable") == 0)
  {
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpScalable::GetTypeId ()));
  }
else if (transport_prot.compare ("TcpHtcp") == 0)
  {
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHtcp::GetTypeId ()));
  }
else if (transport_prot.compare ("TcpVeno") == 0)
  {
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVeno::GetTypeId ()));
  }
else if (transport_prot.compare ("TcpBic") == 0)
  {
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpBic::GetTypeId ()));
  }
else if (transport_prot.compare ("TcpYeah") == 0)
  {
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpYeah::GetTypeId ()));
  }
else if (transport_prot.compare ("TcpIllinois") == 0)
  {
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpIllinois::GetTypeId ()));
  }
else if (transport_prot.compare ("TcpWestwood") == 0)
  { // the default protocol type in ns3::TcpWestwood is WESTWOOD
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
    Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
  }
else if (transport_prot.compare ("TcpWestwoodPlus") == 0)
  {
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
    Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
    Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
  }

  NS_LOG_INFO ("Build star topology.");
  PointToPointHelper p2pHelper;
  p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2pHelper.SetChannelAttribute ("Delay", StringValue ("10ms"));

  PointToPointStarHelper starSource (nSpokes, p2pHelper);
  PointToPointStarHelper starSink (nSpokes, p2pHelper);

  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper stack;
  starSource.InstallStack(stack);
  starSink.InstallStack(stack);

  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  starSource.AssignIpv4Addresses(address);
  address.SetBase ("10.2.1.0", "255.255.255.0");
  starSink.AssignIpv4Addresses(address);

  NS_LOG_INFO ("Linking the hubs.");
  NodeContainer hubs;
  hubs.Add(starSource.GetHub());
  hubs.Add(starSink.GetHub());

  p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  p2pHelper.SetChannelAttribute ("Delay", StringValue ("20ms"));

  NetDeviceContainer devices;
  devices = p2pHelper.Install (hubs);

  NS_LOG_INFO ("Assigning IP to hubs for P2P Link");
  address.SetBase ("10.3.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  double startTime = 1.0;
  double endTime = 60.0;

  NS_LOG_INFO ("Creating Source Apps.");
  uint16_t port = 50000;

  BulkSendHelper source ("ns3::TcpSocketFactory", Address());
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  
  ApplicationContainer sourceApps;  
  
  for (uint32_t i = 0; i < starSource.SpokeCount (); ++i)
  {
    AddressValue remoteAddress (InetSocketAddress (starSink.GetSpokeIpv4Address (i), port));
    source.SetAttribute ("Remote", remoteAddress);
    sourceApps.Add (source.Install (starSource.GetSpokeNode (i)));
  }
  
  sourceApps.Start (Seconds (2.0));
 
  NS_LOG_INFO ("Creating Sink Apps.");

  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
 
  ApplicationContainer sinkApps;

  for (uint32_t i = 0; i < starSink.SpokeCount (); ++i)
  {
    sinkApps.Add (packetSinkHelper.Install (starSink.GetSpokeNode (i)));
  }

  sinkApps.Start (Seconds (startTime));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint32_t totalRx = 0;

  Simulator::Stop(Seconds(endTime));

  Simulator::Run ();

  std::ofstream outFile;
  std::string fileName = transport_prot + std::to_string(nSpokes) + ".txt";
  outFile.open(fileName, std::ios::out);

  for(uint32_t i = 0; i < nSpokes; ++i)
  {
    Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApps.Get(i));
    uint32_t bytesReceived = sink->GetTotalRx();
    totalRx += bytesReceived;
    std::cout << "Sink " << i << "\tTotalRx: " << bytesReceived*8e-6 << "Mb";
    outFile << "Sink " << i << "\tTotalRx: " << bytesReceived*8e-6 << "Mb";
    std::cout << "\tThroughput: " << (bytesReceived*8e-6)/endTime << "Mbps" << std::endl;
    outFile << "\tThroughput: " << (bytesReceived*8e-6)/endTime << "Mbps" << std::endl;
  }

  std::cout << std::endl;
  outFile << std::endl;
  std::cout << "Totals\tTotalRx: " << totalRx*8e-6  << "Mb";
  outFile << "Totals\tTotalRx: " << totalRx*8e-6  << "Mb";
  std::cout << "\tThroughput: " << (totalRx*8e-6)/endTime << "Mbps" << std::endl;
  outFile << "\tThroughput: " << (totalRx*8e-6)/endTime << "Mbps" << std::endl;  

  outFile.close();

  Simulator::Destroy ();
  return 0;
}
