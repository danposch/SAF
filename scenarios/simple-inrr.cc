/**
 * Copyright (c) 2015 Daniel Posch (Alpen-Adria Universität Klagenfurt)
 *
 * This file is part of the ndnSIM extension for Stochastic Adaptive Forwarding (SAF).
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndn-all.hpp"

#include "../extensions/fw/saf.h"
#include "../extensions/utils/extendedglobalroutinghelper.h"
#include "../extensions/fw/competitors/inrr/oracle.h"

using namespace ns3;

int main(int argc, char* argv[])
{

  CommandLine cmd;
  cmd.Parse (argc, argv);

  //parse the topology
  AnnotatedTopologyReader topologyReader ("", 5);
  topologyReader.SetFileName ("topologies/inrr.top");
  topologyReader.Read();

  //grep the nodes
  Ptr<Node> streamer0 = Names::Find<Node>("ContentDst0");
  Ptr<Node> streamer1 = Names::Find<Node>("ContentDst1");
  Ptr<Node> streamer2 = Names::Find<Node>("ContentDst2");
  Ptr<Node> streamer3 = Names::Find<Node>("ContentDst3");

  Ptr<Node> provider0 = Names::Find<Node>("ContentSrc0");
  Ptr<Node> provider1 = Names::Find<Node>("ContentSrc1");

	NodeContainer inrr_nodes;
	inrr_nodes.Add(streamer0);
	inrr_nodes.Add(streamer1);
	inrr_nodes.Add(streamer2);
	inrr_nodes.Add(streamer3);

  inrr_nodes.Add(Names::Find<Node>("Router0"));
  inrr_nodes.Add(Names::Find<Node>("Router1"));
  inrr_nodes.Add(Names::Find<Node>("Router2"));
  inrr_nodes.Add(Names::Find<Node>("Router3"));
  inrr_nodes.Add(Names::Find<Node>("Router4"));
  inrr_nodes.Add(Names::Find<Node>("Router5"));

  // Install NDN stack on all nodes
  ns3::ndn::StackHelper ndnHelper;

  ndnHelper.setCsSize(1); // disable caches
  ndnHelper.Install (provider0);
  ndnHelper.Install (provider1);

  ndnHelper.setCsSize(2500); // set chache to 2500 objects
  ndnHelper.Install (inrr_nodes);

  //install iNRR on routers and clients
	for(int i = 0; i < inrr_nodes.size(); i++)
	{
		//we use NS3 names to globally make the nodes available as well as their forwards.
		//its a hack, but its the only way ns3/ndnSIM allows to implement an oracle.
		std::string oldname = Names::FindName (inrr_nodes.Get (i));
    std::string newname = "StrategyNode" + boost::lexical_cast<std::string>(i);
		Names::Rename (oldname, newname);
		nfd::fw::StaticOracaleContainer::getInstance()->insertNode(newname, inrr_nodes.Get (i));
    ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::Oracle>(inrr_nodes.Get (i),"/");
		Names::Rename (newname, oldname);
	}

  // Install NDN applications
  std::string prefix0 = "/provider0";
  std::string prefix1 = "/provider1";

  //install consumer application on the streamers
  ns3::ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute ("Frequency", StringValue ("250")); //roughly 2Mbps
  consumerHelper.SetAttribute ("Randomize", StringValue ("uniform"));

  //install first streamer
  consumerHelper.SetPrefix (prefix0);
  consumerHelper.Install(streamer0).Start(Seconds(2)); // set start time to 2 seconds after sim start

  //install second streamer
  consumerHelper.SetPrefix(prefix1);
  consumerHelper.Install(streamer1).Start(Seconds(2)); // set start time to 2 seconds after sim start

  //install third streamer
  consumerHelper.SetPrefix(prefix0);
  consumerHelper.Install(streamer2);

  //install fourth streamer
  consumerHelper.SetPrefix(prefix1);
  consumerHelper.Install(streamer3);

  //install producer application on the providers
  ns3::ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));

  //install first producer
  producerHelper.SetPrefix (prefix0);
  producerHelper.Install (provider0);

  //install second producer
  producerHelper.SetPrefix (prefix1);
  producerHelper.Install (provider1);

   // Installing "extended" global routing interface on all nodes
  ns3::ndn::ExtendedGlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  //add routing prefixes to the providers
  ndnGlobalRoutingHelper.AddOrigins(prefix0, provider0);
  ndnGlobalRoutingHelper.AddOrigins(prefix1, provider1);

	//install default routes to all nodes. This is needed for iNRRR Strategy!
	ndnGlobalRoutingHelper.AddOriginsForAllUsingNodeIds ();

  // Calculate and install FIBs
  ns3::ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes ();

  //clean up the simulation
  Simulator::Stop (Seconds(600)); //runs for 10 min.
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_UNCOND("Simulation completed!");
  return 0;
}
