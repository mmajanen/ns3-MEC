/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 *
 * Modications for MEC testing purposes by
 * Mikko Majanen <mikko.majanen@vtt.fi>
 * Copyright (c) 2018 VTT Technical Research Centre of Finland Ltd.
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
//#include "ns3/gtk-config-store.h"

#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+MEC+EPC. A MEC server is attached to each
 * eNodeB (or PGW). MEC can run both UDP and TCP based applications.
 * 
 */

NS_LOG_COMPONENT_DEFINE ("LteMecExample");

int
main (int argc, char *argv[])
{

  LogComponentEnable("EpcEnbApplication", LOG_LEVEL_ALL);
  LogComponentEnable("MecServerApplication", LOG_LEVEL_ALL);
  LogComponentEnable("PointToPointEpcHelper", LOG_LEVEL_ALL);
  LogComponentEnable("PacketSink", LOG_LEVEL_ALL);
  LogComponentEnable("LteMecExample", LOG_LEVEL_ALL);
  
  for(int i=0;i<argc;i++){
    NS_LOG_LOGIC("argv["<<i<<"] = " << argv[i]);
  }
  
  uint16_t numberOfNodes = 1;
  double simTime = 25.1;
  double distance = 60.0;
  double interPacketInterval = 100;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of UE nodes", numberOfNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
  NS_LOG_LOGIC("remote host addr: " << remoteHostAddr);
  Ipv4Address pgwAddr = internetIpIfaces.GetAddress(0);
  NS_LOG_LOGIC("PGW addr: " << pgwAddr);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(1); //only one eNB
  Ptr<Node> enb = enbNodes.Get(0);
  ueNodes.Create(numberOfNodes);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector(70.0, 180.0, 10));  
  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator(positionAlloc);
  enbmobility.Install(enbNodes);

  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i<numberOfNodes; i++) {
    uePositionAlloc->Add (Vector (1.0+10*i, 170.0, 1.5));
  }
  MobilityHelper uemobility;
  uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  uemobility.SetPositionAllocator(uePositionAlloc);
  uemobility.Install(ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numberOfNodes; i++)
      {
        //attach one UE per eNB:
        //lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i));
        // side effect: the default EPS bearer will be activated

        //attach all UEs to the same eNB:
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(0));
      }


  // Install and start applications and packet sinks
  uint16_t tcpPort = 1234;
  uint16_t udpPort = 2000;
  ApplicationContainer clientApps; //UDP and TCP apps on UE
  ApplicationContainer serverApps; //the packet sinks

  bool useMecOnEnb = true; //true --> MEC on eNB
  bool useMecOnPgw = false;//true --> MEC on PGW (if not on eNB)
  //if both above are false, no MEC, packet sinks are on remote host
  
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      ++udpPort;
      ++tcpPort;
      PacketSinkHelper udpPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), udpPort));
      PacketSinkHelper tcpPacketSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), tcpPort));

      Ptr<Node> targetHost;
      Ipv4Address targetAddr;
      if(useMecOnEnb){
        targetHost = epcHelper->GetMecNode(); //TODO: works only for 1 eNB !
        targetAddr = epcHelper->GetMecAddr();
      }
      else if(useMecOnPgw){
        targetHost = pgw;
        targetAddr = pgwAddr;
      }
      else {
        targetHost = remoteHost;
        targetAddr = remoteHostAddr;
      }
        
      serverApps.Add (udpPacketSinkHelper.Install (targetHost));
      serverApps.Add (tcpPacketSinkHelper.Install (targetHost));

      NS_LOG_LOGIC("targetHostAddr = " << targetAddr);
      UdpClientHelper ulClient (InetSocketAddress(targetAddr, udpPort)); 
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
      ulClient.SetAttribute ("PacketSize", UintegerValue(1400)); 

      clientApps.Add (ulClient.Install (ueNodes.Get(u)));
      
      BulkSendHelper TcpClient("ns3::TcpSocketFactory", InetSocketAddress(targetAddr, tcpPort));
      
      TcpClient.SetAttribute("MaxBytes", UintegerValue(1000000));
      clientApps.Add(TcpClient.Install(ueNodes.Get(u))); 
     
    }
  
  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));
  clientApps.Stop (Seconds (10.01)); 
  lteHelper->EnableTraces ();

  /*
  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lte-mec");
  */
  
  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();
  
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/
 
  flowMonitor->SerializeToXmlFile("flow_stats_mec.xml", true, true);
  
  Simulator::Destroy();
  return 0;

}

