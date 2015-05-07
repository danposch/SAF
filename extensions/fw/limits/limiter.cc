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

#include "limiter.h"

using namespace nfd;
using namespace nfd::fw;

Limiter::Limiter(double maxTokens)
{
  this->maxTokens = maxTokens;
  tokens = 0.0;
  addTokens(std::max(1.0,maxTokens * INITIAL_TOKENS));
}

Limiter::~Limiter ()
{
}

double Limiter::addTokens (double tokens)
{
  double ret = 0.0;
  this->tokens += tokens;

  if(this->tokens > maxTokens)
  {
    ret = this->tokens - maxTokens;
    this->tokens = maxTokens;
  }
  return ret;
}

bool Limiter::tryConsumeToken()
{
  if(tokens >= 1)
  {
    tokens-=1;
    return true;
  }
  return false;
}

bool Limiter::isFull ()
{
  if(tokens >= maxTokens)
    return true;

  return false;
}

void Limiter::setNewMaxTokenSize(double maxTokens)
{
  this->maxTokens = maxTokens;

  if(tokens > maxTokens)
    tokens = maxTokens;
}
