#ifndef FACECONTROLLERENTRY_H
#define FACECONTROLLERENTRY_H

#include <map>
#include <string>
#include <math.h>

#include "ns3/timer.h"
#include "../../../utils/parameterconfiguration.h"

#define DEFAULT_DELAY "1000.0ms"

namespace nfd
{
namespace fw
{

enum OMPIFType {Client = 0, Router = 1, Invalid = 2};

class FaceControllerEntry
{
public:

  FaceControllerEntry(std::string prefix);
  FaceControllerEntry(const FaceControllerEntry& other);
  FaceControllerEntry operator=(const FaceControllerEntry& other);

  std::string getPrefix();

  int determineOutFace(int inFace_id, double rvalue);

  void expiredInterest(int face_id);
  void satisfiedInterest(int face_id, ns3::Time delay, OMPIFType type);

  void addAlternativeGoodFace(int face_id, OMPIFType type);

protected:

  std::string prefix;

  typedef std::map
  < int, /*face ID*/
    ns3::Time /* delay of face*/
  > GoodFaceMap;

  GoodFaceMap map;

};

}
}

#endif // FACECONTROLLERENTRY_H
