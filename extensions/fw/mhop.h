#ifndef SAFHOPSTATISTIC_H
#define SAFHOPSTATISTIC_H

#include "mratio.h"
#include <vector>

namespace nfd {
namespace fw {

class MHop : public Mratio
{
public:
  MHop(std::vector<int> faces, int max_hops);

  virtual void logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data);

protected:
  int curMaxHop;
};

}
}
#endif // SAFHopSTATISTIC_H
