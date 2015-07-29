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

#ifndef PIC_H
#define PIC_H

#define ALPHA_PIC 0.9 //as defined in OMCCRF paper

namespace nfd
{
namespace fw
{

class PIC
{
public:
  PIC();
  ~PIC();
  void update();

  void increase();
  void decrease();

  double getWeight();

protected:
  int pic;
  double avg_pic;
  double weight;

};

}
}
#endif // PIC_H
