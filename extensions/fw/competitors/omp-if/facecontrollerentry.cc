#include "facecontrollerentry.h"

using namespace nfd;
using namespace nfd::fw;

FaceControllerEntry::FaceControllerEntry(std::string prefix)
{
  this->prefix = prefix;
}

FaceControllerEntry::FaceControllerEntry(const FaceControllerEntry &other)
{
  this->prefix = other.prefix;
  this->map = other.map;
}

FaceControllerEntry FaceControllerEntry::operator=(const FaceControllerEntry& other)
{
  this->prefix = other.prefix;
  this->map = other.map;

  return *this;
}

std::string FaceControllerEntry::getPrefix()
{
  return prefix;
}

int FaceControllerEntry::determineOutFace (int inFace_id, double rvalue)
{
  if (map.size () == 1)
  {
    return map.begin ()->first; //typcial case for router
  }
  else if(map.size () > 0)
  {
    double w_sum = 0.0;
    for(GoodFaceMap::iterator it = map.begin (); it != map.end (); it++)
    {
      if(it->first != inFace_id)
      {
        w_sum+= 1.0/(double) it->second.GetMilliSeconds (); //sum weights
      }
    }

    if(w_sum == 0.0)
      return DROP_FACE_ID; // just in case..

    double sum = 0.0;
    for(GoodFaceMap::iterator it = map.begin (); it != map.end (); it++)
    {
      if(it->first != inFace_id)
      {
        sum += 1.0 / ((double) it->second.GetMilliSeconds () * (double) (w_sum));
        if(rvalue <=  sum)//inverse transfrom sampling on normalized values
        {
          return it->first;
        }
      }
    }
  }
  return DROP_FACE_ID;
}

void FaceControllerEntry::expiredInterest(int face_id)
{
  GoodFaceMap::iterator it = map.find (face_id);
  if(it != map.end ())
  {
    map.erase (it);
  }
}

void FaceControllerEntry::satisfiedInterest(int face_id, ns3::Time delay, OMPIFType type)
{
  GoodFaceMap::iterator it = map.find (face_id);
  if(it != map.end ())
  {
    //there is no averaging mechanism suggested in the paper, thus we use an exponential moving average
    map[face_id] = ns3::MilliSeconds(round((0.9*(double)map[face_id].GetMilliSeconds () + 0.1*(double)delay.GetMilliSeconds ())));
  }
  else if(type == OMPIFType::Client || map.size () == 0) //only add if client if map size > 0
    map[face_id] = delay; //new entry
}

void FaceControllerEntry::addAlternativeGoodFace(int face_id, OMPIFType type)
{
  //check if face is known
  if(map.find (face_id) != map.end ())
    return; //already known

  //else add alternative path with some default rtt-delay
  //we can not estimate the delay, and the paper does not provide a default value so we just take 1 sec as default!
  if(type == OMPIFType::Client || map.size () == 0)
    map[face_id] = ns3::Time(DEFAULT_DELAY);
}


