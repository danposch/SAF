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

#ifndef ORACLE_H
#define ORACLE_H

#include "face/face.hpp"
#include "fw/strategy.hpp"
#include "oraclecontainer.h"

#include "boost/shared_ptr.hpp"
#include "map"
#include "ns3/node.h"
#include "ns3/names.h"
#include "limits.h"
#include "ns3/ndnSIM/model/ndn-net-device-face.hpp"

#include "ns3/ptr.h"
#include "ns3/channel.h"
#include "ns3/random-variable.h"
#include "ns3/net-device.h"

/* This Strategy implements the ideal Nearst Replica Routing iNRR as proposed by Rossini and Rossi in: Coupling Caching and Forwarding: Benefits, Analysis, and Implementation.*/

namespace nfd
{
namespace fw
{

class Oracle : public nfd::fw::Strategy
{
public:
  Oracle(Forwarder &forwarder, const Name &name = STRATEGY_NAME);

  virtual ~Oracle();
  virtual void afterReceiveInterest(const nfd::Face& inFace, const ndn::Interest& interest,shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry);
  virtual void beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,const nfd::Face& inFace, const ndn::Data& data);
  virtual void beforeExpirePendingInterest(shared_ptr< pit::Entry > pitEntry);

  static const Name STRATEGY_NAME;

protected:

  std::vector<ns3::ndn::NetDeviceFace *> getAllInNetDeviceFaces(shared_ptr<pit::Entry> pitEntry);

  std::vector<ns3::Ptr<ns3::NetDevice> > getAllNetDevicesFromNode(ns3::Ptr<ns3::Node> node);

  ns3::Ptr<ns3::Node> findNearestReplica(std::vector<ns3::Ptr<ns3::Node> > visitedNodes, std::vector<ns3::Ptr<ns3::Node> > relevantNodes, shared_ptr<pit::Entry> pitEntry, int remainingSteps);
  ns3::Ptr<ns3::Node> getCounterpart(ns3::Ptr<ns3::NetDevice> face, ns3::Ptr<ns3::Node> node);
  ns3::Ptr<ns3::Node> getCounterpart(ns3::ndn::NetDeviceFace* face, ns3::Ptr<ns3::Node> node);

  bool checkCacheHit(shared_ptr<pit::Entry> pitEntry, ns3::Ptr<ns3::Node> node);

  ns3::UniformVariable randomVariable;

  ns3::Ptr<ns3::Node> node;

  Forwarder* forwarder;
};

}
}
#endif
