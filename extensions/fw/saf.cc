#include "saf.h"

using namespace nfd;
using namespace nfd::fw;

const Name SAF::STRATEGY_NAME("ndn:/localhost/nfd/strategy/saf");

SAF::SAF(Forwarder &forwarder, const Name &name) : Strategy(forwarder, name)
{
  const FaceTable& ft = getFaceTable();
  int prefixComponets = 0;
  engine = boost::shared_ptr<SAFEngine>(new SAFEngine(ft, prefixComponets));
}

SAF::~SAF()
{
}

void SAF::afterReceiveInterest(const Face& inFace, const Interest& interest ,shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
{
  /* Attention!!! interest != pitEntry->interest*/ // necessary to emulate NACKs in ndnSIM2.0
  /* interst could be /NACK/suffix, while pitEntry->getInterest is /suffix */

  //find + exclude inface(s) and already tried outface(s)
  std::vector<int> originInFaces = getAllInFaces(pitEntry);
  std::vector<int> alreadyTriedFaces; // keep them empty for now and check if nack or retransmission?

  std::string prefix = interest.getName().get(0).toUri();
  if(prefix.compare("NACK") == 0)
  {
    //fprintf(stderr, "Received Nack %s on face[%d]\n", interest.getName().toUri().c_str(), inFace.getId ());
    engine->logNack(inFace, pitEntry->getInterest());
    alreadyTriedFaces = getAllOutFaces(pitEntry);
  }

  const Interest int_to_forward = pitEntry->getInterest();
  int nextHop = engine->determineNextHop(int_to_forward, alreadyTriedFaces, fibEntry);
  while(nextHop != DROP_FACE_ID && (std::find(originInFaces.begin (),originInFaces.end (), nextHop) == originInFaces.end ()))
  {
    bool success = engine->tryForwardInterest (int_to_forward, getFaceTable ().get (nextHop));

    /*DISABLING LIMITS FOR NOW*/
    success = true; // as not used in the SAF paper.

    if(success)
    {
      //fprintf(stderr, "Transmitting %s on face[%d]\n", int_to_forward.getName().toUri().c_str(), nextHop);
      sendInterest(pitEntry, getFaceTable ().get (nextHop));
      return;
    }

    engine->logNack((*getFaceTable ().get(nextHop)), pitEntry->getInterest());
    alreadyTriedFaces.push_back (nextHop);
    nextHop = engine->determineNextHop(int_to_forward, alreadyTriedFaces, fibEntry);
  }
  //fprintf(stderr, "Rejecting Interest %s\n", int_to_forward.getName ().toUri ().c_str ());
  engine->logRejectedInterest(pitEntry, nextHop);
  rejectPendingInterest(pitEntry);
}

void SAF::beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  engine->logSatisfiedInterest (pitEntry, inFace, data);
  Strategy::beforeSatisfyInterest (pitEntry,inFace, data);
}

void SAF::beforeExpirePendingInterest(shared_ptr< pit::Entry > pitEntry)
{
  //fprintf(stderr, "Timeout %s\n", pitEntry->getInterest().getName().toUri().c_str());
  engine->logExpiredInterest(pitEntry);
  Strategy::beforeExpirePendingInterest (pitEntry);
}

std::vector<int> SAF::getAllInFaces(shared_ptr<pit::Entry> pitEntry)
{
  std::vector<int> faces;
  const nfd::pit::InRecordCollection records = pitEntry->getInRecords();

  for(nfd::pit::InRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
  {
    if(! (*it).getFace()->isLocal())
      faces.push_back((*it).getFace()->getId());
  }
  return faces;
}

std::vector<int> SAF::getAllOutFaces(shared_ptr<pit::Entry> pitEntry)
{
  std::vector<int> faces;
  const nfd::pit::OutRecordCollection records = pitEntry->getOutRecords();

  for(nfd::pit::OutRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
    faces.push_back((*it).getFace()->getId());

  return faces;
}

