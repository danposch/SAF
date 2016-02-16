#ifndef SAFDELAYSTATISTIC_H
#define SAFDELAYSTATISTIC_H

#include "mratio.h"
#include <vector>

namespace nfd {
namespace fw {

class MDelay : public Mratio
{
public:
  MDelay(std::vector<int> faces, int max_delay_ms);

  virtual void logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data);

protected:
  //time::steady_clock::Duration curMaxDelay;
  int curMaxDelay;
};

}
}
#endif // SAFDELAYSTATISTIC_H
