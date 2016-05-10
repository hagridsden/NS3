
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
#include "ns3/packet-sink.h"

//#include "ns3/gtk-config-store.h"


using namespace ns3;

FILE * output = fopen ("scratch/Throughput_9_FTP_Voice_UE.txt", "w");

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
	FlowMonitor is used to calculate the stats for Summary.
 */

/*Function to compute the throughput at the server taken as argument*/
void get_throughput(Ptr<PacketSink> sink_a, Ptr<PacketSink> sink_b)
{
	Time t = Simulator::Now();								//current time of simulator
	if(t.GetSeconds()==0.0)
	{
		Simulator::Schedule(Seconds(0.1), &get_throughput, sink_a, sink_b);
		return;
	}
	//std::cout << t.GetSeconds() << std::endl;

	int bytesTotal = sink_a->GetTotalRx();					//get no of packets received
    float mbs_1 = (bytesTotal*8.0)/1000/t.GetSeconds();		//throughput = bytes*8/time/1000 Kbps


    bytesTotal = sink_b->GetTotalRx();					//get no of packets received
    t = Simulator::Now();								//current time of simulator
    float mbs_2 = (bytesTotal*8.0)/1000/t.GetSeconds();		//throughput = bytes*8/time/1000 Kbps
   // std::cout << "Throughput(Sink 2): " << mbs_2 << " Kbps" << std::endl << std::endl;


    fprintf(output, "%f\t%f\t%f\n", t.GetSeconds(), mbs_1, mbs_2);

    if(t.GetSeconds()<60.0)
    {
    	Simulator::Schedule(Seconds(0.1), &get_throughput, sink_a, sink_b);
    }

  }

void getStats (FlowMonitorHelper* fmhelper, Ptr<FlowMonitor> flowMon)
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
			std::cout<<"Tx Packets = " << stats->second.txPackets<< " : "<<stats->second.txBytes<< " Bytes" <<std::endl;
			std::cout<<"Rx Packets = " << stats->second.rxPackets<< " : "<<stats->second.rxBytes<< " Bytes" <<std::endl;
			std::cout<<"Duration		: "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<std::endl;

			std::cout<<"Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;

			float thpt = float(stats->second.rxBytes * 8.0) / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024;
			std::cout<<"Throughput: " <<  thpt << " Mbps"<<std::endl;

			std::cout<<"---------------------------------------------------------------------------"<<std::endl;

			txPacketsum += stats->second.txPackets;
			rxPacketsum += stats->second.rxPackets;
			LostPacketsum = txPacketsum-rxPacketsum;
			DropPacketsum += stats->second.packetsDropped.size();
			Delaysum += stats->second.delaySum.GetSeconds();

		}

		  std::cout<<std::endl<<std::endl;
		  std::cout << "**Combined SUMMARY**"<<std::endl;
		  std::cout << " All Tx Packets: " << txPacketsum << "\n";
		  std::cout << " All Rx Packets: " << rxPacketsum << "\n";
		 // std::cout << " All Delay: " << Delaysum  << "\n";
		  std::cout << " All Lost/ReTX Packets: " << LostPacketsum << "\n";
		//  std::cout << "  All Drop Packets: " << DropPacketsum << "\n";
		  float sRate = (float)(rxPacketsum*100) / txPacketsum;
		  float fRate = (float)(LostPacketsum*100) / txPacketsum ;
		  std::cout << " Packet Delivery Efficiency % : " << sRate << "%" << "\n";
		  std::cout << " Packets Lost/ReTX % : " << fRate << "%" << "\n"<<std::endl<<std::endl;


}


NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");

int main (int argc, char *argv[])
{

  uint16_t numberOfNodes = 7;
  double simTime = 60;				// in seconds
  double distance = 60.0;

  // Calculate the ADU size
  Header* temp_header = new Ipv4Header ();
  uint32_t ip_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("IP Header size is: " << ip_header);
  delete temp_header;
  temp_header = new TcpHeader ();
  uint32_t tcp_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("TCP Header size is: " << tcp_header);
  delete temp_header;
  uint32_t mtu_bytes=1500;
  uint32_t tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);
  NS_LOG_LOGIC ("TCP ADU size is: " << tcp_adu_size);

  
  CommandLine cmd;
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  //Log Enable
  //LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
  //LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  //LogComponentEnable ("OnOffApplication", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_FUNCTION));

  std::cout << "1. Configuring. Done!" << std::endl;

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

	std::cout << "2. Installing LTE+EPC+Remotehost. Done!" << std::endl;

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("2Mbps")));		// P-to-P Bandwidth
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(1);
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
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(0));
      }

  std::cout << "4. Installing eNodeBs and UEs.. Done! "<< std::endl;

   uint16_t port_ftp = 20;         // FTP data
   uint16_t port_voip = 6000;      // VoIP data
 	 // uint16_t port_voip_ctrl = 6001; // VoIP control

   Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));

  OnOffHelper onoff_ftp("ns3::TcpSocketFactory", Address(InetSocketAddress (internetIpIfaces.GetAddress (1), port_ftp)));
  onoff_ftp.SetAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
  onoff_ftp.SetAttribute("PacketSize", UintegerValue(1000));

  ApplicationContainer ftp_server1, ftp_server2, ftp_server3, ftp_server4, ftp_server5, ftp_server6, ftp_server7;


  OnOffHelper onoff_VOIP("ns3::UdpSocketFactory", Address(InetSocketAddress (internetIpIfaces.GetAddress (1), port_voip)));
  onoff_VOIP.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  onoff_VOIP.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  onoff_VOIP.SetAttribute("DataRate", DataRateValue(DataRate("64Kbps")));
  onoff_VOIP.SetAttribute("PacketSize", UintegerValue(200));
  ApplicationContainer VOIP_user1, VOIP_user2, VOIP_user3, VOIP_user4, VOIP_user5, VOIP_user6, VOIP_user7, VOIP_user8, VOIP_user9, VOIP_user10, VOIP_user11, VOIP_user12, VOIP_user13, VOIP_user14, VOIP_user15;
  ApplicationContainer VOIP_user16, VOIP_user17, VOIP_user18, VOIP_user19, VOIP_user20;
  ApplicationContainer VOIP_user21, VOIP_user22, VOIP_user23, VOIP_user24, VOIP_user25, VOIP_user26, VOIP_user27, VOIP_user28, VOIP_user29;

  // packet sink at remote host
  PacketSinkHelper sink_VOIP ("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port_voip)));
  ApplicationContainer VOIP_client;

  PacketSinkHelper sink_ftp ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port_ftp)));
  ApplicationContainer ftp_client;

 	// NS_LOG_INFO ("Create Applications.");

	 //  FTP (from UE1 to RH)
	  ftp_server1 = onoff_ftp.Install (ueNodes.Get(0));
	  ftp_server1.Start(Seconds(0.0));
	  ftp_server1.Stop(Seconds(simTime));

	  //  FTP (from UE2 to RH)
	  ftp_server2 = onoff_ftp.Install (ueNodes.Get(1));
	  ftp_server2.Start(Seconds(10.0));
	  ftp_server2.Stop(Seconds(simTime));

	  //  FTP (from UE3 to RH)
	  ftp_server3 = onoff_ftp.Install (ueNodes.Get(2));
	  ftp_server3.Start(Seconds(20.0));
	  ftp_server3.Stop(Seconds(simTime));

	   //  FTP (from UE4 to RH)
	   ftp_server4 = onoff_ftp.Install (ueNodes.Get(3));
	   ftp_server4.Start(Seconds(30.0));
	   ftp_server4.Stop(Seconds(simTime));

	   //  FTP (from UE5 to RH)
	 	  ftp_server5 = onoff_ftp.Install (ueNodes.Get(4));
	 	  ftp_server5.Start(Seconds(40.0));
	 	  ftp_server5.Stop(Seconds(simTime));

	 	  //  FTP (from UE6 to RH)
	 	  ftp_server6 = onoff_ftp.Install (ueNodes.Get(5));
	 	  ftp_server6.Start(Seconds(45.0));
	 	  ftp_server6.Stop(Seconds(simTime));

	 	   //  FTP (from UE7 to RH)
	 	   ftp_server7 = onoff_ftp.Install (ueNodes.Get(6));
	 	   ftp_server7.Start(Seconds(50.0));
	 	   ftp_server7.Stop(Seconds(simTime));

	   /*
   // int g=2;

	   //VOIP (from UE1 to RH)
  	  VOIP_user1 = onoff_VOIP.Install (ueNodes.Get(4));
  	  VOIP_user1.Start(Seconds(15));	// starting at t=0s
  	  VOIP_user1.Stop(Seconds(simTime));
  	  //VOIP (from UE2 to RH)
  	  VOIP_user2 = onoff_VOIP.Install (ueNodes.Get(5));
  	  VOIP_user2.Start(Seconds(25));	// starting at t=30s
  	  VOIP_user2.Stop(Seconds(simTime));

  	  //VOIP (from UE3 to RH)
  	  VOIP_user3 = onoff_VOIP.Install (ueNodes.Get(6));
  	  VOIP_user3.Start(Seconds(35));	// starting at t=0s
  	  VOIP_user3.Stop(Seconds(simTime));

  	  //VOIP (from UE4 to RH)
  	  VOIP_user4 = onoff_VOIP.Install (ueNodes.Get(7));
  	  VOIP_user4.Start(Seconds(45));	// starting at t=0s
  	  VOIP_user4.Stop(Seconds(simTime));

  	  //VOIP (from UE5 to RH)
  	  VOIP_user5 = onoff_VOIP.Install (ueNodes.Get(8));
  	  VOIP_user5.Start(Seconds(55));	// starting at t=0s
  	  VOIP_user5.Stop(Seconds(simTime));


  	  //VOIP (from UE6 to RH)
  	  VOIP_user6 = onoff_VOIP.Install (ueNodes.Get(5));
  	  VOIP_user6.Start(Seconds(6*g));	// starting at t=0s
  	  VOIP_user6.Stop(Seconds(simTime));

  	  //VOIP (from UE7 to RH)
  	  VOIP_user7 = onoff_VOIP.Install (ueNodes.Get(6));
  	  VOIP_user7.Start(Seconds(7*g));	// starting at t=0s
  	  VOIP_user7.Stop(Seconds(simTime));

  	  //VOIP (from UE8 to RH)
  	  VOIP_user8 = onoff_VOIP.Install (ueNodes.Get(7));
  	  VOIP_user8.Start(Seconds(8*g));	// starting at t=0s
  	  VOIP_user8.Stop(Seconds(simTime));

  	  //VOIP (from UE9 to RH)
  	  VOIP_user9 = onoff_VOIP.Install (ueNodes.Get(8));
  	  VOIP_user9.Start(Seconds(9*g));	// starting at t=0s
  	  VOIP_user9.Stop(Seconds(simTime));

  	  //VOIP (from UE10 to RH)
  	  VOIP_user10 = onoff_VOIP.Install (ueNodes.Get(9));
  	  VOIP_user10.Start(Seconds(10*g));	// starting at t=0s
  	  VOIP_user10.Stop(Seconds(simTime));

  	  //VOIP (from UE11 to RH)
  	  VOIP_user11 = onoff_VOIP.Install (ueNodes.Get(10));
  	  VOIP_user11.Start(Seconds(11*g));	// starting at t=0s
  	  VOIP_user11.Stop(Seconds(simTime));

  	  //VOIP (from UE12 to RH)
  	  VOIP_user12 = onoff_VOIP.Install (ueNodes.Get(11));
  	  VOIP_user12.Start(Seconds(12*g));	// starting at t=0s
  	  VOIP_user12.Stop(Seconds(simTime));

  	  //VOIP (from UE13 to RH)
  	  VOIP_user13 = onoff_VOIP.Install (ueNodes.Get(12));
  	  VOIP_user13.Start(Seconds(13*g));	// starting at t=0s
  	  VOIP_user13.Stop(Seconds(simTime));

  	  //VOIP (from UE14 to RH)
  	  VOIP_user14 = onoff_VOIP.Install (ueNodes.Get(13));
  	  VOIP_user14.Start(Seconds(14*g));	// starting at t=0s
  	  VOIP_user14.Stop(Seconds(simTime));

  	  //VOIP (from UE15 to RH)
  	  VOIP_user15 = onoff_VOIP.Install (ueNodes.Get(14));
  	  VOIP_user15.Start(Seconds(15*g));	// starting at t=0s
  	  VOIP_user15.Stop(Seconds(simTime));

  	  

	   //VOIP (from UE16 to RH)
	   VOIP_user16 = onoff_VOIP.Install (ueNodes.Get(15));
	   VOIP_user16.Start(Seconds(16*g));	// starting at t=0s
	   VOIP_user16.Stop(Seconds(simTime));

	   //VOIP (from UE17 to RH)
	  	VOIP_user17 = onoff_VOIP.Install (ueNodes.Get(16));
	  	VOIP_user17.Start(Seconds(17*g));	// starting at t=0s
	  	VOIP_user17.Stop(Seconds(simTime));

	  	//VOIP (from UE18 to RH)
	  	VOIP_user18 = onoff_VOIP.Install (ueNodes.Get(17));
	  	VOIP_user18.Start(Seconds(18*g));	// starting at t=0s
	  	VOIP_user18.Stop(Seconds(simTime));

	  	//VOIP (from UE19 to RH)
	  	VOIP_user19 = onoff_VOIP.Install (ueNodes.Get(18));
	  	VOIP_user19.Start(Seconds(19*g));	// starting at t=0s
	  	VOIP_user19.Stop(Seconds(simTime));

	  	//VOIP (from UE20 to RH)
    	  VOIP_user20 = onoff_VOIP.Install (ueNodes.Get(19));
	   	  VOIP_user20.Start(Seconds(20*g));	// starting at t=0s
	   	  VOIP_user20.Stop(Seconds(simTime));

	    //VOIP (from UE21 to RH)
	   	  VOIP_user21 = onoff_VOIP.Install (ueNodes.Get(20));
	   	  VOIP_user21.Start(Seconds(21*g));	// starting at t=0s
	   	  VOIP_user21.Stop(Seconds(simTime));

	   	  //VOIP (from UE22 to RH)
	   	  VOIP_user22 = onoff_VOIP.Install (ueNodes.Get(21));
	   	  VOIP_user22.Start(Seconds(22*g));	// starting at t=0s
	   	  VOIP_user22.Stop(Seconds(simTime));

	   	  //VOIP (from UE23 to RH)
	   	  VOIP_user23 = onoff_VOIP.Install (ueNodes.Get(22));
	   	  VOIP_user23.Start(Seconds(23*g));	// starting at t=0s
	   	  VOIP_user23.Stop(Seconds(simTime));

	   	  //VOIP (from UE24 to RH)
	   	  VOIP_user24 = onoff_VOIP.Install (ueNodes.Get(23));
	   	  VOIP_user24.Start(Seconds(24*g));	// starting at t=0s
	   	  VOIP_user24.Stop(Seconds(simTime));

	   	  //VOIP (from UE25 to RH)
	   	  VOIP_user25 = onoff_VOIP.Install (ueNodes.Get(24));
	   	  VOIP_user25.Start(Seconds(25*g));	// starting at t=0s
	   	  VOIP_user25.Stop(Seconds(simTime));

	   	  //VOIP (from UE26 to RH)
	   	  VOIP_user26 = onoff_VOIP.Install (ueNodes.Get(25));
	   	  VOIP_user26.Start(Seconds(26*g));	// starting at t=0s
	   	  VOIP_user26.Stop(Seconds(simTime));

	   	  //VOIP (from UE27 to RH)
	   	  VOIP_user27 = onoff_VOIP.Install (ueNodes.Get(26));
	   	  VOIP_user27.Start(Seconds(27*g));	// starting at t=0s
	   	  VOIP_user27.Stop(Seconds(simTime));

	   	  //VOIP (from UE28 to RH)
	   	  VOIP_user28 = onoff_VOIP.Install (ueNodes.Get(27));
	   	  VOIP_user28.Start(Seconds(28*g));	// starting at t=0s
	   	  VOIP_user28.Stop(Seconds(simTime));

	   	  //VOIP (from UE29 to RH)
	   	  VOIP_user29 = onoff_VOIP.Install (ueNodes.Get(28));
	   	  VOIP_user29.Start(Seconds(29*g));	// starting at t=0s
	   	  VOIP_user29.Stop(Seconds(simTime));
*/
	   	 // Packet sink at RH for FTP packets
	  		  ftp_client = sink_ftp.Install (remoteHostContainer.Get (0));
	  		  ftp_client.Start (Seconds (0.0));
	  		  ftp_client.Stop (Seconds(simTime));

	  		  // Packet sink at RH for VOIP packets
	  		  VOIP_client = sink_VOIP.Install (remoteHostContainer.Get (0));
	  		  VOIP_client.Start(Seconds(0.0));
	  		  VOIP_client.Stop (Seconds(simTime));

	  std::cout << "5. Running Applications. Done!" << std::endl;

      lteHelper->EnableTraces ();
      p2ph.EnablePcapAll("7_FTP_UE");

//FLOW MONITOR

  FlowMonitorHelper fmhelper;
  Ptr<FlowMonitor> flowMon;

  flowMon = fmhelper.InstallAll();
     std::cout << "6. Setting FlowMonitor. Done!" << std::endl;


   Ptr<PacketSink> sink_a = DynamicCast<PacketSink> (ftp_client.Get (0));
   Ptr<PacketSink> sink_b = DynamicCast<PacketSink> (VOIP_client.Get (0));

   //schedule the function once and then it will schedule itself repeatedly
   	Simulator::Schedule(Seconds(0.0), &get_throughput, sink_a,sink_b);

   	flowMon->SerializeToXmlFile("results_7_FTP_UE.xml", true, true);

   	Simulator::Stop(Seconds(simTime));
   	Simulator::Run();

  /*
   *
   * //Application FTP
	  uint32_t totalRxBytesCounter_ftp = 0;
	 // Ptr <Application> app1 = ftp_client.Get (0);
	 // Ptr <PacketSink> pktSink1 = DynamicCast <PacketSink> (app1);
	  totalRxBytesCounter_ftp += sink_a->GetTotalRx ();

	  float goodPut = (totalRxBytesCounter_ftp/Simulator::Now ().GetSeconds ())/1000;
	  std::cout << "\nGoodput FTP: " << goodPut*8/1000 << " Mbps" << std::endl;
//	  Time t=Simulator::Now();
//	  fprintf(output,"%f\t%f\n",t.GetSeconds(),goodPut);
*/

   getStats(&fmhelper ,flowMon);


  Simulator::Destroy();
  return 0;
}


