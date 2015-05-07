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

#ifndef MRATIO_H
#define MRATIO_H

#include "safstatisticmeasure.h"
#include <iostream>
namespace nfd
{
namespace fw
{

/**
 * @brief The Mratio class implements the measure S(F_i), U(F_i) as indicated in the SAF paper.
 */
class Mratio : public SAFStatisticMeasure
{
public:
  Mratio(std::vector<int> faces);

  virtual void logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data);
  virtual void logExpiredInterest(shared_ptr<pit::Entry> pitEntry);
  virtual void logNack(const Face& inFace, const Interest& interest);
  virtual void logRejectedInterest (shared_ptr<pit::Entry> pitEntry, int face_id);

protected:


};

}
}
#endif // MRATIO_H
