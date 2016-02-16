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

#ifndef SAFMEASUREFACTORY_H
#define SAFMEASUREFACTORY_H

#include <cstddef>

#include "safstatisticmeasure.h"
#include "mdelay.h"
#include "tuple"

namespace nfd
{
namespace fw
{

class SAFMeasureFactory
{
public:
  static SAFMeasureFactory* getInstance();

  boost::shared_ptr<SAFStatisticMeasure> getMeasure(std::string name, std::vector<int> faces);
  void registerMeasure(std::string prefix, SAFStatisticMeasure::MeasureType type);
  void registerAttribute(std::string prefix, std::string attribute, std::string value);

protected:

  SAFMeasureFactory();
  static SAFMeasureFactory* instance;

  std::map<
  std::string /*prefix*/,
  SAFStatisticMeasure::MeasureType /*measure type*/
  > typedef MeasureMap;

  MeasureMap mmap;

  std::map<
  std::string /*prefix*/,
  std::map< std::string /* attrib*/, std::string /*value>*/ >
  > typedef MeasureAttributeMap;

  MeasureAttributeMap amap;

};

}
}
#endif // SAFMEASUREFACTORY_H
