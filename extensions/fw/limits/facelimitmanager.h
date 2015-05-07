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

#ifndef FACELIMITMANAGER_H
#define FACELIMITMANAGER_H

#define INTEREST_PACKET_SIZE 100
#define DATA_PACKET_SIZE 4096 + INTEREST_PACKET_SIZE
#define TOKEN_FILL_INTERVALL 10 //ms
//#define BUCKET_SIZE 10.0
#define NACK_RETURN_TOKEN 0.5

#include "fw/face-table.hpp"
#include "boost/shared_ptr.hpp"
#include <limits>

#include "ns3-dev/ns3/point-to-point-module.h"
#include "ns3-dev/ns3/network-module.h"
#include "ns3/simple-ref-count.h"
#include "ns3/ndnSIM/model/ndn-net-device-face.hpp"

#include "limiter.h"

namespace nfd
{
namespace fw
{
class FaceLimitManager
{
public:
  FaceLimitManager(shared_ptr< Face > face);

  bool addNewPrefix(std::string content_prefix);
  bool tryForwardInterest(std::string prefix);
  void receivedNack(std::string prefix);

protected:

  void newToken();
  std::vector<std::string> getAllNonFullBuckets();

  shared_ptr< Face > face;

  /* map for storing forwarding stats for all faces */
  typedef std::map
    < std::string, /*prefix*/
      boost::shared_ptr<Limiter> /*Limiter*/
    > LimitMap;

  LimitMap bMap;

  uint64_t getPhysicalBitrate(shared_ptr< Face > face);
  double tokenGenRate;
  ns3::EventId newTokenEvent;
};

}
}

#endif // FACELIMITMANAGER_H
