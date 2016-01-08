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

#ifndef PARAMETERCONFIGURATION_H
#define PARAMETERCONFIGURATION_H

#include <cstddef>
#include <map>
#include <string>

//default parameters can be overriden:
#define P_LAMBDA 0.35 // rate to adapt reliability threshold
#define P_UPDATE_INTERVALL 0.5 // duration of a period in seconds
#define P_MAX_LAYERS 1 //number of layers per content
#define P_DROP_FACE_ID -1 // the id of the virtual dropping face
#define P_RELIABILITY_THRESHOLD_MIN 0.75 // the minimum required reliablity
#define P_RELIABILITY_THRESHOLD_MAX 0.999 // the maximum enforced reliability
#define P_HISTORY_SIZE 6 // sample size of the windows for the statisticmeasure class.
#define P_PREFIX_COMPONENT 0 // component that seperates the prefix from the remaining name

//some additional defines
#define DROP_FACE_ID -1
#define FACE_NOT_FOUND -1

/**
 * @brief The ParameterConfiguration class is used to set/get parameters to configure SAF.
 * The class uses a singleton pattern.
 */
class ParameterConfiguration
{
public:

  /**
   * @brief returns the singleton instance.
   * @return
   */
  static ParameterConfiguration* getInstance();

  /**
   * @brief sets a parameter
   * @param param_name the name of the parameter.
   * @param value the value of the parameter.
   */
  void setParameter(std::string param_name, double value);

  /**
   * @brief gets a parameter.
   * @param para_name the name of the parameter.
   * @return
   */
  double getParameter(std::string para_name);

protected:  
  ParameterConfiguration();

  static ParameterConfiguration* instance;

  std::map<
  std::string /*param name*/,
  double /*param value*/
  > typedef ParameterMap;

  ParameterMap pmap;
};

#endif // PARAMETERCONFIGURATION_H
