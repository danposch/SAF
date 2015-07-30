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

using namespace ns3;

int main(int argc, char* argv[])
{

  CommandLine cmd;
  cmd.Parse (argc, argv);

  //parse the topology
  AnnotatedTopologyReader topologyReader ("", 5);
  topologyReader.SetFileName ("topologies/example.top");
  topologyReader.Read();

  //grep the nodes
  Ptr<Node> streamer0 = Names::Find<Node>("ContentDst0");
  Ptr<Node> streamer1 = Names::Find<Node>("ContentDst1");

  Ptr<Node> provider0 = Names::Find<Node>("ContentSrc0");
  Ptr<Node> provider1 = Names::Find<Node>("ContentSrc1");

  NodeContainer routers;
  routers.Add(Names::Find<Node>("Router0"));
  routers.Add(Names::Find<Node>("Router1"));
  routers.Add(Names::Find<Node>("Router2"));
  routers.Add(Names::Find<Node>("Router3"));
  routers.Add(Names::Find<Node>("Router4"));
  routers.Add(Names::Find<Node>("Router5"));

  // Install NDN stack on all nodes
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(1); // disable caches

  ndnHelper.Install (streamer0);
  ndnHelper.Install (streamer1);

  ndnHelper.Install (provider0);
  ndnHelper.Install (provider1);

  ndnHelper.Install (routers);

  //install SAF on routers
  ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::SAF>(routers,"/");

  // Install NDN applications
  std::string prefix0 = "/provider0";
  std::string prefix1 = "/provider1";

  //install consumer application on the streamers
  ns3::ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute ("Frequency", StringValue ("250")); //roughly 2Mbps
  consumerHelper.SetAttribute ("Randomize", StringValue ("uniform"));

  //install first streamer
  consumerHelper.SetPrefix (prefix0);
  consumerHelper.Install(streamer0);

  //install second streamer
  consumerHelper.SetPrefix(prefix1);
  consumerHelper.Install(streamer1);

  //install producer application on the providers
  ns3::ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));

  //install first producer
  producerHelper.SetPrefix (prefix0);
  producerHelper.Install (provider0);

  //install second producer
  producerHelper.SetPrefix (prefix1);
  producerHelper.Install (provider1);

   // Installing global routing interface on all nodes
  ns3::ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  //add routing prefixes to the providers
  ndnGlobalRoutingHelper.AddOrigins(prefix0, provider0);
  ndnGlobalRoutingHelper.AddOrigins(prefix1, provider1);

  // Calculate and install FIBs
  ns3::ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes ();

  //clean up the simulation
  Simulator::Stop (Seconds(600)); //runs for 10 min.
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_UNCOND("Simulation completed!");
  return 0;
}
