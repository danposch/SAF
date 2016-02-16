#include "mratio.h"

using namespace nfd;
using namespace nfd::fw;

Mratio::Mratio(std::vector<int> faces) : SAFStatisticMeasure(faces)
{
  this->type = MeasureType::MThroughput;
}

void Mratio::logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  int ilayer = SAFStatisticMeasure::determineContentLayer(pitEntry->getInterest());
  stats[ilayer].satisfied_requests[inFace.getId ()] += 1;
}

void Mratio::logExpiredInterest(shared_ptr<pit::Entry> pitEntry)
{
  int ilayer = SAFStatisticMeasure::determineContentLayer(pitEntry->getInterest());

  const nfd::pit::OutRecordCollection records = pitEntry->getOutRecords();
  for(nfd::pit::OutRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
  {
    //fprintf(stderr, "Timeout loged on face %d\n",(*it).getFace()->getId());
    stats[ilayer].unsatisfied_requests[(*it).getFace()->getId()] += 1;
  }
}

void Mratio::logNack(const Face &inFace, const Interest &interest)
{
  int ilayer = SAFStatisticMeasure::determineContentLayer(interest);
  stats[ilayer].unsatisfied_requests[inFace.getId()] += 1;
}

void Mratio::logRejectedInterest (shared_ptr<pit::Entry> pitEntry, int face_id)
{
  //fprintf(stderr, "Rejected Interest logged on face %d\n",DROP_FACE_ID);

  int ilayer = SAFStatisticMeasure::determineContentLayer(pitEntry->getInterest());

  if(face_id == DROP_FACE_ID)
    stats[ilayer].satisfied_requests[face_id] += 1;
  else
    stats[ilayer].unsatisfied_requests[face_id] += 1;
}
