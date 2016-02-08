#include "ompif.h"

using namespace nfd;
using namespace nfd::fw;

const Name OMPIF::STRATEGY_NAME("ndn:/localhost/nfd/strategy/ompif");

OMPIF::OMPIF(Forwarder &forwarder, const Name &name) : Strategy(forwarder, name)
{
  prefixComponents = ParameterConfiguration::getInstance ()->getParameter ("PREFIX_COMPONENT");
  type = OMPIFType::Invalid;
}

OMPIF::~OMPIF()
{
  for(InterestTimeOutMap::iterator it = timeOutMap.begin (); it != timeOutMap.end (); it++)
    ns3::Simulator::Cancel (it->second); // cancel all open events
}

void OMPIF::afterReceiveInterest(const Face& inFace, const Interest& interest ,shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
{
  if(!fibEntry->hasNextHops()) // check if nexthop(s) exist(s)
  {
    //rejectPendingInterest(pitEntry); this would create a NACK OMPIF does not use them, so just drop.
    return;
  }

  std::string prefix = extractContentPrefix(pitEntry->getInterest().getName());

  //lets start with OMPIF strategy here
  if(fMap.find (prefix) != fMap.end ()) //search FC-entry
  {
    boost::shared_ptr<FaceControllerEntry> entry = fMap[prefix];

    double rvalue = randomVariable.GetValue ();
    int nextHop = entry->determineOutFace(inFace.getId (),rvalue); //get nexthop from entry
    if(nextHop != DROP_FACE_ID)
    {
      //check if we observe the time out of the first arrived interest for this pit entry
      if(timeOutMap.find (pitEntry) == timeOutMap.end ())
      {
        //if not start observing as client rtx may keep pit-entries alive... and we may never see an expire event..
        int ltime = boost::chrono::duration_cast<boost::chrono::milliseconds>(interest.getInterestLifetime ()).count();
        timeOutMap[pitEntry] = ns3::Simulator::Schedule(ns3::MilliSeconds(ltime), &OMPIF::onInterestTimeOut, this, pitEntry, nextHop);
      }
      pitMap[pitEntry][nextHop] = ns3::Simulator::Now (); //update delay measurment entry for this face
      sendInterest(pitEntry, getFaceTable ().get (nextHop));
      return;
    }
    //else continue with broadcast probing
  }

  fib::NextHopList nexthops = fib::NextHopList(fibEntry->getNextHops()); //create copy as we need to shuffle
  std::random_shuffle(nexthops.begin (), nexthops.end(), randomShuffle);

  DelayFaceMap dMap;

   for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it)
   {
     shared_ptr<Face> outFace = it->getFace();
     if (pitEntry->canForwardTo(*outFace))
     {
       dMap[outFace->getId()] = ns3::Simulator::Now ();
        this->sendInterest(pitEntry, outFace);
     }
   }

   if(pitEntry->hasUnexpiredOutRecords())
     pitMap[pitEntry]=dMap; //only store in measurement map if forwarding was possible
}

void OMPIF::onInterestTimeOut(shared_ptr<pit::Entry> pitEntry, int face)
{
  std::string prefix = extractContentPrefix(pitEntry->getName());

  if(fMap.find (prefix) != fMap.end ()) //
  {
    fMap[prefix]->expiredInterest(face); // this will mark the face as unreliable
  }

  InterestTimeOutMap::iterator iit = timeOutMap.find (pitEntry); // erase the entry in the even table
  if(iit != timeOutMap.end ())
  {
    timeOutMap.erase (iit); // cancel time event obsvering the first interest for this entry
  }
}

void OMPIF::beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  InterestTimeOutMap::iterator iit = timeOutMap.find (pitEntry);
  if(iit != timeOutMap.end ())
  {
    ns3::Simulator::Cancel (iit->second); //cancel time event if we were observing it
    timeOutMap.erase (iit); //and delete it from map
  }

  PitMap::iterator pit = pitMap.find (pitEntry);
  if(pit == pitMap.end ())
  {
    return; //due to late straggle timer of ndnsim forwarder, just return in this case
  }

  DelayFaceMap dmap = pitMap.find (pitEntry)->second;
  for(DelayFaceMap::iterator it = dmap.begin (); it != dmap.end (); it++)
  {
    if(it->first == inFace.getId ())
    {
      std::string prefix = extractContentPrefix(data.getName());
      if(fMap.find (prefix) == fMap.end ())
      {
        fMap[prefix] = boost::shared_ptr<FaceControllerEntry>(new FaceControllerEntry(prefix)); //add new entry
      }
      fMap[prefix]->satisfiedInterest(inFace.getId (), ns3::Simulator::Now ()-it->second, type); // log satisfied interest
      break;
    }
  }
  pitMap.erase (pit);
  Strategy::beforeSatisfyInterest(pitEntry,inFace,data);
}

void OMPIF::beforeExpirePendingInterest(shared_ptr< pit::Entry > pitEntry)
{
  std::string prefix = extractContentPrefix(pitEntry->getName());

  if(fMap.find (prefix) != fMap.end ()) //
  {
    const nfd::pit::OutRecordCollection records = pitEntry->getOutRecords();
    for(nfd::pit::OutRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
    {
      fMap[prefix]->expiredInterest((*it).getFace()->getId());
    }
  }

  PitMap::iterator it = pitMap.find (pitEntry);
  if(it != pitMap.end ())
  {
    pitMap.erase (it);
  }

  InterestTimeOutMap::iterator iit = timeOutMap.find (pitEntry);
  if(iit != timeOutMap.end ())
  {
    ns3::Simulator::Cancel (iit->second); //cancel time event if we were observing it
    timeOutMap.erase (iit); //and delete it from map
  }

  Strategy::beforeExpirePendingInterest(pitEntry);
}

void OMPIF::onUnsolicitedData(const Face& inFace, const Data& data)
{
  std::string prefix = extractContentPrefix(data.getName());

  //check if data prefix is known
  if(fMap.find (prefix) != fMap.end ()) //
  {
    fMap[prefix]->addAlternativeGoodFace(inFace.getId (), type);
  }
  Strategy::onUnsolicitedData (inFace,data);
}

std::string OMPIF::extractContentPrefix(nfd::Name name)
{
  std::string prefix = "";
  for(int i=0; i <= prefixComponents; i++)
  {
    prefix.append ("/");
    prefix.append (name.get (i).toUri ());
  }
  return prefix;
}
