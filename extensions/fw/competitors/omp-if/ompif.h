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

/*This Strategy implements the OMP-IF as presented in Udugama et al.: An On-demand Multi-Path Interest Forwarding Strategy for Content Retrievals in CCN*/

#ifndef OMPIF_H
#define OMPIF_H

#include "face/face.hpp"
#include "fw/strategy.hpp"

#include "boost/shared_ptr.hpp"
#include "ns3/random-variable.h"
#include "../../../utils/parameterconfiguration.h"
#include "facecontrollerentry.h"

#include "boost/chrono.hpp"
#include <algorithm>

namespace nfd
{
namespace fw
{

class OMPIF : public nfd::fw::Strategy
{
public:

  virtual ~OMPIF();
  virtual void afterReceiveInterest(const nfd::Face& inFace, const ndn::Interest& interest,shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry);
  virtual void beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,const nfd::Face& inFace, const ndn::Data& data);
  virtual void beforeExpirePendingInterest(shared_ptr< pit::Entry > pitEntry);
  virtual void onUnsolicitedData(const Face& inFace, const Data& data);

  static const Name STRATEGY_NAME;

  protected:

  OMPIF(Forwarder &forwarder, const Name &name = STRATEGY_NAME);

  std::string extractContentPrefix(nfd::Name name);

  void onInterestTimeOut(shared_ptr<pit::Entry> pitEntry, int face);

  ns3::UniformVariable randomVariable;

  static int randomShuffle(int i) { ns3::UniformVariable r;
                                    return r.GetInteger (0,i-1);}

  typedef std::map<
  std::string /*prefix*/,
  boost::shared_ptr<FaceControllerEntry>
  > FaceControllerMap;

  FaceControllerMap fMap;

  typedef std::map<
  shared_ptr< pit::Entry >,
  ns3::EventId
  > InterestTimeOutMap;

  InterestTimeOutMap timeOutMap;

  typedef std::map<
  int /*face_id*/,
  ns3::Time> DelayFaceMap;

  typedef std::map<
  shared_ptr< pit::Entry >,
  DelayFaceMap> PitMap;

  PitMap pitMap;

  int prefixComponents;

  OMPIFType type;
};

}
}

#endif // OMPIF_H
