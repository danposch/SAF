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

#ifndef ORACLECONTAINER_H
#define ORACLECONTAINER_H

#include "face/face.hpp"
#include "fw/strategy.hpp"

#include "boost/shared_ptr.hpp"
#include "map"
#include "ns3/node.h"

#include "ns3/ptr.h"
namespace nfd
{
namespace fw
{

class StaticOracaleContainer
{
public:

  static StaticOracaleContainer* getInstance();

  void insertNode(const std::string& name, ns3::Ptr<ns3::Node> n);
  ns3::Ptr<ns3::Node> getNode(const std::string& name);

  void insertForwarder(int node_id, nfd::Forwarder* forwarder);
  nfd::Forwarder* getForwarder(int node_id);

protected:

  StaticOracaleContainer();

  typedef std::map<
  std::string, /*name*/
  ns3::Ptr<ns3::Node> /*node*/
  > NodeMap;

  typedef std::map<
  int , /*node id*/
  nfd::Forwarder* /*forwarder*/
  > ForwarderMap;

  NodeMap m;
  ForwarderMap fw_map;

  static StaticOracaleContainer* instance;

};

}
}
#endif
