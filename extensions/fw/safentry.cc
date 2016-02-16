#include "safentry.h"

using namespace nfd;
using namespace nfd::fw;

SAFEntry::SAFEntry(std::vector<int> faces, shared_ptr<fib::Entry> fibEntry, std::string prefix)
{
  this->fibEntry = fibEntry;
  this->faces = faces;
  initFaces();

  smeasure = SAFMeasureFactory::getInstance ()->getMeasure (prefix, faces);
  ftable = boost::shared_ptr<SAFForwardingTable>(new SAFForwardingTable(this->faces, this->preferedFaces));
  fallbackCounter = 0;
}

void SAFEntry::initFaces ()
{
  const fib::NextHopList& nexthops = fibEntry->getNextHops();
  for(fib::NextHopList::const_iterator it = nexthops.begin (); it != nexthops.end (); it++)
  {
    std::vector<int>::iterator face = std::find(faces.begin (),faces.end (), (*it).getFace()->getId());
    if(face != faces.end ())
    {
      //fprintf(stderr, "costs=%d\n",it->getCost());
      preferedFaces[*face]=it->getCost ();
    }
  }
}

int SAFEntry::determineNextHop(const Interest& interest, std::vector<int> alreadyTriedFaces)
{
  return ftable->determineNextHop (interest,alreadyTriedFaces);
}

void SAFEntry::update()
{
  smeasure->update(ftable->getCurrentReliability ());
  ftable->update (smeasure);
  //ftable->crossLayerAdaptation (smeasure);

  /*if(!evaluateFallback())
    ftable->update (smeasure);
  else
    ftable = boost::shared_ptr<SAFForwardingTable>(new SAFForwardingTable(this->faces, this->preferedFaces));*/
}

void SAFEntry::logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  smeasure->logSatisfiedInterest (pitEntry,inFace,data);
}

void SAFEntry::logExpiredInterest(shared_ptr< pit::Entry > pitEntry)
{
  smeasure->logExpiredInterest (pitEntry);
}

void SAFEntry::logNack(const Face& inFace, const Interest& interest)
{
  smeasure->logNack (inFace, interest);
}

void SAFEntry::logRejectedInterest(shared_ptr<pit::Entry> pitEntry, int face_id)
{
  smeasure->logRejectedInterest(pitEntry, face_id);
}

bool SAFEntry::evaluateFallback()
{
  bool fallback = false;
  bool increaseFallback = true;

  if(smeasure->getTotalForwardedInterests (0) == 0)
    return false;

  for(std::vector<int>::iterator it=faces.begin (); it != faces.end (); ++it)
  {
    if(*it == DROP_FACE_ID)
      continue;

    //NS_LOG_UNCOND("forwarded=" << smeasure->getForwardedInterests(*it, 0) << ", linkReliability=" << smeasure50->getLinkReliability (*it, 0));
    if( smeasure->getForwardedInterests (*it, 0) > 0 && smeasure->getFaceReliability (*it, 0) > 0)
    {
      increaseFallback = false;
      break;
    }
  }

  if(increaseFallback)
    fallbackCounter++;
  else if (fallbackCounter > 0)
    fallbackCounter--;

  if(fallbackCounter >= 10.0 / ParameterConfiguration::getInstance ()->getParameter ("UPDATE_INTERVALL"))
  {
    fallbackCounter = 0;
    fallback = true;
  }

  return fallback;
}

void SAFEntry::addFace(shared_ptr<Face> face)
{
  //no mutex needed we are in an event-based simulator

  //pthread_mutex_lock( &mutex);
  smeasure->addFace(face);
  ftable->addFace (face);
  //pthread_mutex_unlock( &mutex);
}

void SAFEntry::removeFace(shared_ptr<Face> face)
{
  //no mutex needed we are in an event-based simulator

  //pthread_mutex_lock( &mutex);
  ftable->removeFace (face);
  smeasure->removeFace (face);
  //pthread_mutex_unlock( &mutex);
}
