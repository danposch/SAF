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

#ifndef SAFFORWARDINGTABLE_H
#define SAFFORWARDINGTABLE_H

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include "ns3/random-variable.h"

#include "../utils/parameterconfiguration.h"
#include "safstatisticmeasure.h"

#include "fw/face-table.hpp"
#include "iostream"
#include "climits"
#include "ns3/log.h"

#define MAX_OBSERVATION_PERIODS 10.0

namespace nfd
{
namespace fw
{

/**
 * @brief The SAFForwardingTable class represents a single forwarding table
 */
class SAFForwardingTable
{
public:

  /**
   * @brief creates a new forwarding table.
   * @param faceIds faces in this table
   * @param preferedFacesIds prefered faces are used to distribute the initial forwarding chances.
   */
  SAFForwardingTable(std::vector<int> faceIds, std::map<int,int> preferedFacesIds = std::map<int,int>());

  /**
   * @brief determines the next hop of a given interest
   * @param interest the interest
   * @param alreadyTriedFaces faces  that have been already tried.
   * @return
   */
  int determineNextHop(const Interest& interest, std::vector<int> alreadyTriedFaces);

  /**
   * @brief update operation for the forwarding table called at the end of each period.
   * @param smeasure the statistic measure object that logged the traffic
   */
  void update(boost::shared_ptr<SAFStatisticMeasure> smeasure);

  /**
   * @brief enables cross layer adaptation. EXPERIMENTAL and DISABLED
   * @param smeasure
   */
  void crossLayerAdaptation(boost::shared_ptr<SAFStatisticMeasure> smeasure);

  /**
   * @brief provides the current reliability threshold for each layer.
   * @return
   */
  std::map<int /*layer*/,double/*reliabilty*/> getCurrentReliability(){return this->curReliability;}

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
  void initTable();
  std::map<int, double> calcInitForwardingProb(std::map<int, int> preferedFacesIds, double gamma);
  std::map<int, double> minHop(std::map<int, int> preferedFacesIds);

  int determineRowOfFace(int face_uid, boost::numeric::ublas::matrix<double> tab, std::vector<int> faces);
  int determineRowOfFace(int face_uid);
  boost::numeric::ublas::matrix<double> removeFaceFromTable (int faceId, boost::numeric::ublas::matrix<double> tab, std::vector<int> faces);
  boost::numeric::ublas::matrix<double> normalizeColumns(boost::numeric::ublas::matrix<double> m);

  int chooseFaceAccordingProbability(boost::numeric::ublas::matrix<double> m, int ilayer, std::vector<int> faceList);

  void probeColumn(std::vector<int> faces, int layer, boost::shared_ptr<SAFStatisticMeasure> smeasure);

  void decreaseReliabilityThreshold(int layer);
  void increaseReliabilityThreshold(int layer);
  void updateReliabilityThreshold(int layer, bool increase);

  int getDroppingLayer();

  boost::numeric::ublas::matrix<double> table;
  std::vector<int> faces;
  std::map<int /*faceId*/,int/*costs/metric*/> preferedFaces;
  std::map<int /*layer*/,double/*reliabilty*/> curReliability;
  ns3::UniformVariable randomVariable;

  std::map<int /*layer*/,int/*steps_left*/> observed_layers;
};

}
}
#endif // SAFFORWARDINGTABLE_H
