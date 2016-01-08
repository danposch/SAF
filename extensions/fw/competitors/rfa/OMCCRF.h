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

/*This Strategy implements the RFA as presented in Carofiglio et al.: Optimal Multipath Congestion Control and Request Forwarding in Information-Centric Networks*/

#ifndef OMCCRF_H
#define OMCCRF_H

#include "face/face.hpp"
#include "fw/strategy.hpp"
#include "pic.h"

#include "boost/shared_ptr.hpp"
#include "ns3/random-variable.h"
#include "../../../utils/parameterconfiguration.h"

namespace nfd
{
namespace fw
{

class OMCCRF : public nfd::fw::Strategy
{
public:
  OMCCRF(Forwarder &forwarder, const Name &name = STRATEGY_NAME);

  virtual ~OMCCRF();
  virtual void afterReceiveInterest(const nfd::Face& inFace, const ndn::Interest& interest,shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry);
  virtual void beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,const nfd::Face& inFace, const ndn::Data& data);
  virtual void beforeExpirePendingInterest(shared_ptr< pit::Entry > pitEntry);

  static const Name STRATEGY_NAME;

protected:

  typedef std::map
    < int, /*face ID*/
      boost::shared_ptr<PIC> /*Pending Interest Count*/
    > FacePicEntryMap;

  typedef std::map
    < std::string, /*prefix*/
      FacePicEntryMap
    > PrefixMap;

  std::string extractContentPrefix(nfd::Name name);
  std::vector<int> getAllOutFaces(shared_ptr<pit::Entry> pitEntry);
  std::vector<int> getAllInFaces(shared_ptr<pit::Entry> pitEntry);

  boost::shared_ptr<PIC> findPICEntry(int face_id, std::string prefix);

  PrefixMap pmap;

  ns3::UniformVariable randomVariable;

  typedef std::map<
  std::string /*interest name*/,
  std::list<int> /*known infaces*/
  > KnownInFaceMap;

  KnownInFaceMap inFaceMap;

  bool isRtx(const nfd::Face& inFace, const ndn::Interest&interest);
  void addToKnownInFaces(const nfd::Face& inFace, const ndn::Interest&interest);
  void clearKnownFaces(const ndn::Interest&interest);

  int prefixComponents;
};


}
}
#endif

