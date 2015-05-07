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

#ifndef SAFENGINE_H
#define SAFENGINE_H

#include "fw/face-table.hpp"
#include "ns3/event-id.h"
#include <vector>
#include "limits/facelimitmanager.h"
#include "ns3/names.h"
#include "ns3/log.h"
#include "safentry.h"

namespace nfd
{
namespace fw
{

/**
 * @brief The SAFEngine class is the forwarding engine for SAF.
 * It manages multiple FWT as it considers different contents and for each content different layers.
 */
class SAFEngine
{
public:

  /**
   * @brief creates a new SAF enginem
   * @param table all faces of the current node
   * @param prefixComponentNumber the number of name components that specify a distinct content/prefix.
   */
  SAFEngine(const nfd::FaceTable& table, unsigned int prefixComponentNumber);

  /**
   * @brief determines the next hop for a given interest.
   * @param interest the interest
   * @param alreadyTriedFaces already tried faces
   * @param fibEntry the corresponding fib-entry
   * @return
   */
  int determineNextHop(const Interest& interest, std::vector<int> alreadyTriedFaces, shared_ptr<fib::Entry> fibEntry);

  /**
   * @brief tries to forwarded an interest via a given face.
   * @param interest the interest
   * @return true if the resources are left, else false.
   */
  bool tryForwardInterest(const Interest& interest, shared_ptr<Face>);

  /**
   * @brief logs a satisfied interest.
   * @param pitEntry the corresponding pit-entry
   * @param inFace the face that fulfilled the request
   * @param data the received data packet
   */
  void logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data);

  /**
   * @brief logs an expired interest.
   * @param itEntry the corresponding pit-entry
   */
  void logExpiredInterest(shared_ptr< pit::Entry > pitEntry);

  /**
   * @brief logs a NACK.
   * @param inFace the face that received the NACK
   * @param interest the NACK
   */
  void logNack(const Face& inFace, const Interest& interest);

  /**
   * @brief logs a rejected interest.
   * @param pitEntry the corresponding pit-entry
   * @param face_id the id of the face that rejected the interest.
   */
  void logRejectedInterest(shared_ptr<pit::Entry> pitEntry, int face_id);

protected:
  void initFaces(const nfd::FaceTable& table);
  std::string extractContentPrefix(nfd::Name name);
  void determineNodeName(const nfd::FaceTable& table);
  std::vector<int> faces;

  void update();

  typedef std::map
    < std::string, /*content-prefix*/
      boost::shared_ptr<SAFEntry> /*forwarding prob. table*/
    > SAFEntryMap;

  SAFEntryMap entryMap;

  typedef std::map
    < int, /*face ID*/
      boost::shared_ptr<FaceLimitManager> /*face limit manager*/
    > FaceLimitMap;

  FaceLimitMap fbMap;

  ns3::EventId updateEventFWT;

  unsigned int prefixComponentNumber;
  std::string nodeName;
};


}
}
#endif // SAFENGINE_H
