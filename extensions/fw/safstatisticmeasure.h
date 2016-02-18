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


#ifndef SAFSTATISTICMEASURE_H
#define SAFSTATISTICMEASURE_H

#include "../utils/parameterconfiguration.h"
#include <boost/shared_ptr.hpp>
#include "fw/strategy.hpp"
#include <vector>
#include <map>
#include "ns3/log.h"
#include <math.h>
#include <list>

#define INIT_VARIANCE 1000 // inital variance

namespace nfd
{
namespace fw
{

class SAFStatisticMeasure
{

public:

  enum MeasureType {MThroughput = 0, MDelay = 1, MHop = 2, UNKOWN = 99};

  ~SAFStatisticMeasure();

  /**
   * @brief logs a satisfied interest.
   * @param pitEntry the corresponding pit-entry
   * @param inFace the face that fulfilled the request
   * @param data the received data packet
   */
  virtual void logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data) = 0;

  /**
   * @brief logs an expired interest.
   * @param itEntry the corresponding pit-entry
   */
  virtual void logExpiredInterest(shared_ptr<pit::Entry> pitEntry) = 0;

  /**
   * @brief logs a NACK.
   * @param inFace the face that received the NACK
   * @param interest the NACK
   */
  virtual void logNack(const Face& inFace, const Interest& interest) = 0;

  /**
   * @brief logs a rejected interest.
   * @param pitEntry the corresponding pit-entry
   * @param face_id the id of the face that rejected the interest.
   */
  virtual void logRejectedInterest(shared_ptr<pit::Entry> pitEntry, int face_id) = 0;

  /**
   * @brief update function called at the end of a period.
   * @param reliability_t a map containing the reliability threshold for each content layer
   */
  virtual void update(std::map<int, double> reliability_t);

  /**
   * @brief returns the set of reliable faces for a given reliability threshold.
   * @param layer the layer under investigation
   * @param reliability_t the reliability threshold
   * @return
   */
  virtual std::vector<int> getReliableFaces(int layer, double reliability_t);

  /**
   * @brief returns the set of unreliable faces for a given reliability threshold.
   * @param layer the layer under investigation
   * @param reliability_t the reliability threshold
   * @return
   */
  virtual std::vector<int> getUnreliableFaces(int layer, double reliability_t);

  /**
   * @brief gets the reliability of a face.
   * @param face_id the face id.
   * @param layer the layer under investigation
   * @return
   */
  double getFaceReliability(int face_id, int layer);

  /**
   * @brief gets the total number of forwarded interests for a given layer.
   * @param layer the layer
   * @return
   */
  int getTotalForwardedInterests(int layer){return stats[layer].total_forwarded_requests;}

  /**
   * @brief gets the forwarded interests for a face and a given layer.
   * @param face_id the face id
   * @param layer the layer
   * @return
   */
  int getForwardedInterests(int face_id, int layer){return getS(face_id, layer) + getU(face_id, layer);}

  /**
   * @brief returns alpha for a given face.
   * @param face_id the face
   * @param layer the layer
   * @return
   */
  double getAlpha(int face_id, int layer);

  /**
   * @brief returns an EMA from alpha for a given face.
   * @param face_id the face
   * @param layer the layer
   * @return
   */
  double getEMAAlpha(int face_id, int layer);

  /**
   * @brief gets the number of satisfied interests for a given face
   * @param face_id the face
   * @param layer the layer
   * @return
   */
  int getS(int face_id, int layer){return stats[layer].last_satisfied_requests[face_id];}

  /**
   * @brief gets the number of unsatisfied interests for a given face
   * @param face_id the face
   * @param layer the layer
   * @return
   */
  int getU(int face_id, int layer){return stats[layer].last_unsatisfied_requests[face_id];}

  /**
   * @brief gets the number ST fraction for a given face
   * @param face_id the face
   * @param layer the layer
   * @return
   */
  double getUT(int face_id, int layer){return ((double) getU(face_id, layer)) / ((double)getTotalForwardedInterests(layer));}

  /**
   * @brief gets the number UT fraction for a given face
   * @param face_id the face
   * @param layer the layer
   * @return
   */
  double getST(int face_id, int layer){return ((double) getS(face_id, layer)) / ((double)getTotalForwardedInterests(layer));}

  /**
   * @brief gets the rho parameter for a given layer.
   * @param the layer
   * @return
   */
  double getRho(int layer);

  /**
   * @brief determines the content layer of an Interest.
   * @param interest the interest.
   * @return
   */
  static int determineContentLayer(const Interest& interest);

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
  SAFStatisticMeasure(std::vector<int> faces);

  void calculateTotalForwardedRequests(int layer);
  void calculateLinkReliabilities(int layer, double reliability_t);
  void calculateUnsatisfiedTrafficFractionOfUnreliableFaces (int layer, double reliability_t);
  void calculateActualForwardingProbabilities (int layer);
  void updateVariance(int layer);
  void calculateEMAAlpha(int layer);

  std::vector<int> faces;

  typedef std::map
    < int, /*face id*/
     int /*value to store*/
    > MeasureIntMap;

  typedef std::map
    < int, /*face id*/
     double /*value to store*/
    > MeasureDoubleMap;

  typedef std::map
  <int, /*face id*/
  std::list<int> /*int queue*/
  > MeasureIntList;

  struct SAFMesureStats
  {
    /* variables used for logging*/
    MeasureIntMap satisfied_requests;
    MeasureIntMap unsatisfied_requests;


    /*variables holding information*/
    int total_forwarded_requests;

    MeasureDoubleMap last_reliability;
    MeasureDoubleMap last_actual_forwarding_probs;

    MeasureIntMap last_satisfied_requests;
    MeasureIntMap last_unsatisfied_requests;

    MeasureDoubleMap satisfaction_variance;
    MeasureIntList satisfied_requests_history;

    MeasureDoubleMap ema_alpha;

    SAFMesureStats()
    {
      total_forwarded_requests = 0;
    }

    SAFMesureStats(const SAFMesureStats& other)
    {
      satisfied_requests = other.satisfied_requests;
      unsatisfied_requests = other.unsatisfied_requests;

      total_forwarded_requests = other.total_forwarded_requests;

      last_reliability = other.last_reliability;
      last_actual_forwarding_probs = other.last_actual_forwarding_probs;

      last_satisfied_requests = other.last_satisfied_requests;
      last_unsatisfied_requests = other.last_unsatisfied_requests;

      satisfaction_variance = other.satisfaction_variance;
      satisfied_requests_history = other.satisfied_requests_history;

      ema_alpha = other.ema_alpha;
    }
  };

  typedef std::map
  <int, /*content layer*/
  SAFMesureStats
  >SAFMesureMap;

  SAFMesureMap stats;

  MeasureType type;

};

}
}
#endif // SAFSATISTICMEASURE_H
