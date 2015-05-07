#include "mdelay.h"
#include "climits"
#include <iostream>
#include "../utils/parameterconfiguration.h"

namespace nfd {
namespace fw {

MDelay::MDelay(std::vector<int> faces) : SAFStatisticMeasure(faces)
{
    curMaxDelay = boost::chrono::duration<long int, boost::ratio<1l, 1000000000l> >(
                (long) (ParameterConfiguration::getInstance()->getParameter("MAX_DELAY_MS")) * 1000000);
}

void MDelay::logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  //TODO: check if rtt < max_delay
  time::steady_clock::TimePoint now = time::steady_clock::now();
  std::list<nfd::pit::OutRecord>::const_iterator outRecord = pitEntry->getOutRecord(inFace);

  time::steady_clock::Duration rtt = boost::chrono::duration<long int, boost::ratio<1l, 1000000000l> >(now - outRecord->getLastRenewed());

  if (rtt > curMaxDelay) {
    logExpiredInterest(pitEntry);
  }
  else {
    int ilayer = SAFStatisticMeasure::determineContentLayer(pitEntry->getInterest());
    stats[ilayer].satisfied_requests[inFace.getId ()] += 1;
  }
}

void MDelay::logExpiredInterest(shared_ptr<pit::Entry> pitEntry)
{
  int ilayer = SAFStatisticMeasure::determineContentLayer(pitEntry->getInterest());

  const nfd::pit::OutRecordCollection records = pitEntry->getOutRecords();
  for(nfd::pit::OutRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
  {
    stats[ilayer].unsatisfied_requests[(*it).getFace()->getId()] += 1;
  }
}

}
}


