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

#ifndef SAFENTRY_H
#define SAFENTRY_H

#include "safstatisticmeasure.h"
#include "safforwardingtable.h"
#include "fw/strategy.hpp"
#include "safmeasurefactory.h"

namespace nfd
{
namespace fw
{

/**
 * @brief The SAFEntry class represents a entry for a given prefix in the forwarding engine.
 */

class SAFEntry
{
public:

  /**
   * @brief creates a new entry for given faces and a corresponding fibEntry.
   * @param faces the faces
   * @param fibEntry the fib-entry
   */
  SAFEntry(std::vector<int> faces, shared_ptr<fib::Entry> fibEntry, std::string prefix);

  /**
   * @brief determines the next hop for an interest
   * @param interest the interest
   * @param alreadyTriedFaces already tried faces
   * @return
   */
  int determineNextHop(const Interest& interest, std::vector<int> alreadyTriedFaces);

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

  /**
   * @brief trigges a update for the current entry. Called at the end of each period.
   */
  void update();

  /**
   * @brief addFace
   * @param face
   */
  void addFace(shared_ptr<Face> face);

  /**
   * @brief removeFace
   * @param face
   */
  void removeFace(shared_ptr<Face> face);

protected:

  void initFaces();
  bool evaluateFallback();

  boost::shared_ptr<SAFStatisticMeasure> smeasure;
  boost::shared_ptr<SAFForwardingTable> ftable;

  std::vector<int> faces;
  typedef std::map<
  int/*faceId*/,
  int /*costs*/> PreferedFaceMap;
  PreferedFaceMap preferedFaces;

  shared_ptr<fib::Entry> fibEntry;

  int fallbackCounter;
};

}
}
#endif // SAFENTRY_H
