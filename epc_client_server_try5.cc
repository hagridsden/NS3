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
#include "ns3/flow-monitor-module.h"
#include "ns3/log.h"

//#include "ns3/gtk-config-store.h"


using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */


void ThroughputMonitor (FlowMonitorHelper* fmhelper, Ptr<FlowMonitor> flowMon)
{
		flowMon->CheckForLostPackets();
		std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
		Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());

		uint32_t txPacketsum = 0;
		uint32_t rxPacketsum = 0;
		uint32_t DropPacketsum = 0;
		uint32_t LostPacketsum = 0;
		double Delaysum = 0;

		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
		{
			Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);

			std::cout<<std::endl<<std::endl;
			std::cout<<"Flow ID			: " << stats->first <<" ; "<< fiveTuple.sourceAddress <<"(Port : "<<fiveTuple.sourcePort<<" )"<<" -----> "<<fiveTuple.destinationAddress<<"(Port : "<<fiveTuple.destinationPort<<" )"<<std::endl;
			std::cout<<"Tx Packets = " << stats->second.txPackets<<std::endl;
			std::cout<<"Rx Packets = " << stats->second.rxPackets<<std::endl;
			std::cout<<"Duration		: "<<stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds()<<std::endl;

			std::cout<<"Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
			std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps"<<std::endl;
			std::cout<<"---------------------------------------------------------------------------"<<std::endl;

			txPacketsum += stats->second.txPackets;
			rxPacketsum += stats->second.rxPackets;
			LostPacketsum = txPacketsum-rxPacketsum;
			DropPacketsum += stats->second.packetsDropped.size();
			Delaysum += stats->second.delaySum.GetSeconds();
		}


		  std::cout<<std::endl<<std::endl;
		  std::cout << "**SUMMARY**"<<std::endl;
		  std::cout << "  All Tx Packets: " << txPacketsum << "\n";
		  std::cout << "  All Rx Packets: " << rxPacketsum << "\n";
		  std::cout << "  All Delay: " << Delaysum / txPacketsum << "\n";
		  std::cout << "  All Lost Packets: " << LostPacketsum << "\n";
		  std::cout << "  All Drop Packets: " << DropPacketsum << "\n";
		  std::cout << "  Packets Delivery Ratio: " << ((rxPacketsum * 100) / txPacketsum) << "%" << "\n";
		  std::cout << "  Packets Lost Ratio: " << ((LostPacketsum * 100) / txPacketsum) << "%" << "\n";



			Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon);
	}


NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");

int main (int argc, char *argv[])
{

  uint16_t numberOfNodes = 2;
  double simTime = 1;
  double distance = 60.0;
  double interPacketInterval = 10;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
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

  //Log Enable
  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpClient", LOG_LEVEL_ALL);
  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
  LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);

  std::cout << "1. Configuring. Done!" << std::endl;

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

	std::cout << "2. Installing LTE+EPC+remotehost. Done!" << std::endl;

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

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfNodes);
  ueNodes.Create(numberOfNodes);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfNodes; i++)
    {
      positionAlloc->Add (Vector(distance * i, 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  mobility.Install(ueNodes);

  std::cout << "3. Installing Mobility Models Done!"<< std::endl;

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
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i));
        // side effect: the default EPS bearer will be activated
      }

  std::cout << "4. Installing eNodeBs and UEs: " << numberOfNodes <<". Done! "<< std::endl;

  // Install and start applications on UEs and remote host
//	uint16_t dlPort = 1234;
//	uint16_t ulPort = 1235;
  //	uint16_t otherPort = 1236;

  //ApplicationContainer onOffApp;

	ApplicationContainer clientApps;
	ApplicationContainer serverApps;
	
	
	

	  uint16_t dlPort = 1234;
	  uint16_t ulPort = 2000;
	  

	  for (uint32_t u = 0; u < numberOfNodes; u++)
	  {

	  PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress(ueIpIface.GetAddress (u), dlPort));
	  PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress(ueIpIface.GetAddress (u), ulPort));
	 	 
	  serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
	  serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
	  UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
	  dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
	  dlClient.SetAttribute ("MaxPackets", UintegerValue(100000));
	  dlClient.SetAttribute ("PacketSize", UintegerValue(25));
	  UdpClientHelper ulClient (remoteHostAddr, ulPort);
	  ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
	  ulClient.SetAttribute ("MaxPackets", UintegerValue(100000));
	  ulClient.SetAttribute ("PacketSize", UintegerValue(25));

	  clientApps.Add (ulClient.Install (ueNodes.Get(u)));   //uplink
//R   clientApps.Add (dlClient.Install (remoteHost));		//downlink

	  }
	
	/*
	
	for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
	{
	//   ++otherPort;

	//for voice
	//      ulOnOffHelper.SetAttribute("PacketSize", UintegerValue(172));
	//      ulOnOffHelper.SetAttribute("DataRate",DataRateValue(68800));
	//460800 dr , ps 1316 for video 3686400 , 14745600, 15206400

	OnOffHelper dlonOffHelper("ns3::UdpSocketFactory", InetSocketAddress(internetIpIfaces.GetAddress (1), dlPort++)); //OnOffApplication, UDP traffic ; DL from remote host
	dlonOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=20]"));
	dlonOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
	dlonOffHelper.SetAttribute("DataRate", DataRateValue(DataRate("2Mbps")));
	dlonOffHelper.SetAttribute("PacketSize", UintegerValue(1000));
	//onOffApp.Add(dlonOffHelper.Install(remoteHost));
 
	OnOffHelper ulonOffHelper("ns3::UdpSocketFactory", InetSocketAddress(ueIpIface.GetAddress (0), ulPort++)); //OnOffApplication, UDP traffic,
	ulonOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=20]"));
	ulonOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
	ulonOffHelper.SetAttribute("DataRate", DataRateValue(DataRate("2Mbps")));
	ulonOffHelper.SetAttribute("PacketSize", UintegerValue(1000));
	//onOffApp.Add(ulonOffHelper.Install(remoteHost));

	serverApps.Add (ulonOffHelper.Install(ueNodes.Get(u)));
	clientApps.Add (dlonOffHelper.Install(remoteHost));
	clientApps.Start (Seconds(0.0001));		//TRAFFIC GENERATOR
	clientApps.Stop(Seconds(simTime));

	serverApps.Start (Seconds(0.0001));		//TRAFFIC GENERATOR
	serverApps.Stop(Seconds(simTime));

    }
     
//onOffApp.Start(Seconds(0.01));		//TRAFFIC GENERATOR
//onOffApp.Stop(Seconds(0.1));
  */
	  
	  
	  
  std::cout << "5. Installing Applications. Done!" << std::endl;

  lteHelper->EnableTraces ();
  // Uncomment to enable PCAP tracing
  p2ph.EnablePcapAll("lena-epc-try2");


  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.Install(ueNodes);
  monitor = flowmon.Install(remoteHost);
  //monitor = flowmon.GetMonitor ();

   monitor->SetAttribute("DelayBinWidth", DoubleValue (0.001));

   monitor->SetAttribute("JitterBinWidth", DoubleValue (0.001));

   monitor->SetAttribute("PacketSizeBinWidth", DoubleValue (1000));

   monitor->SerializeToXmlFile("results.xml", true, true);
   std::cout << "6. Setting FlowMonitor. Done!" << std::endl;

  Simulator::Stop(Seconds(simTime));

  Simulator::Run();

  monitor->SerializeToXmlFile ("results_epc_try1.xml",false,false);  //  Print per flow statistics


    ThroughputMonitor(&flowmon ,monitor);

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy();
  return 0;
}

