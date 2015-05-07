#include "safengine.h"

using namespace nfd;
using namespace nfd::fw;

NS_LOG_COMPONENT_DEFINE ("SAFEngine");

SAFEngine::SAFEngine(const FaceTable& table, unsigned int prefixComponentNumber)
{
  initFaces(table);
  this->prefixComponentNumber = prefixComponentNumber;

  updateEventFWT = ns3::Simulator::Schedule(
        ns3::Seconds(ParameterConfiguration::getInstance ()->getParameter ("UPDATE_INTERVALL")), &SAFEngine::update, this);
}

void SAFEngine::initFaces(const nfd::FaceTable& table)
{
  faces.clear ();
  faces.push_back (DROP_FACE_ID);

  fbMap.clear ();

  for(nfd::FaceTable::const_iterator it = table.begin (); it != table.end (); ++it)
  {
    if((*it)->isLocal())
      continue;
    faces.push_back((*it)->getId());
    fbMap[(*it)->getId()] = boost::shared_ptr<FaceLimitManager>(new FaceLimitManager(*it));
  }

  std::sort(faces.begin(), faces.end());
  determineNodeName(table);
}

int SAFEngine::determineNextHop(const Interest& interest, std::vector<int> alreadyTriedFaces, shared_ptr<fib::Entry> fibEntry)
{
  //check if content prefix has been seen
  std::string prefix = extractContentPrefix(interest.getName());

  if(entryMap.find(prefix) == entryMap.end ())
  {
    entryMap[prefix] = boost::shared_ptr<SAFEntry>(new SAFEntry(faces, fibEntry));

    // add buckets for all faces
    for(FaceLimitMap::iterator it = fbMap.begin (); it != fbMap.end (); it++)
    {
      it->second->addNewPrefix(prefix);
    }
  }

  boost::shared_ptr<SAFEntry> entry = entryMap.find(prefix)->second;
  return entry->determineNextHop(interest, alreadyTriedFaces);
}

bool SAFEngine::tryForwardInterest(const Interest& interest, shared_ptr<Face> outFace)
{
  if( dynamic_cast<ns3::ndn::NetDeviceFace*>(&(*outFace)) == NULL) //check if its a NetDevice
  {
    return true;
  }

  std::string prefix = extractContentPrefix(interest.getName());
  SAFEntryMap::iterator it = entryMap.find (prefix);
  if(it == entryMap.end ())
  {
    fprintf(stderr,"Error in SAFEntryLookUp\n");
    return false;
  }
  else
  {
    return fbMap[outFace->getId ()]->tryForwardInterest(prefix);
  }
}

void SAFEngine::update ()
{
  NS_LOG_DEBUG("\nFWT UPDATE at SimTime " << ns3::Simulator::Now ().GetSeconds () << " for Node: '" << nodeName);
  for(SAFEntryMap::iterator it = entryMap.begin (); it != entryMap.end (); ++it)
  {
    NS_LOG_DEBUG("Updating Prefix " << it->first);
    it->second->update();
  }

  updateEventFWT = ns3::Simulator::Schedule(
        ns3::Seconds(ParameterConfiguration::getInstance ()->getParameter ("UPDATE_INTERVALL")), &SAFEngine::update, this);
}

void SAFEngine::logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  std::string prefix = extractContentPrefix(pitEntry->getName());
  SAFEntryMap::iterator it = entryMap.find (prefix);
  if(it == entryMap.end ())
    fprintf(stderr,"Error in SAFEntryLookUp\n");
  else
    it->second->logSatisfiedInterest(pitEntry,inFace,data);
}

void SAFEngine::logExpiredInterest(shared_ptr< pit::Entry > pitEntry)
{
  std::string prefix = extractContentPrefix(pitEntry->getName());
  SAFEntryMap::iterator it = entryMap.find (prefix);
  if(it == entryMap.end ())
    fprintf(stderr,"Error in SAFEntryLookUp\n");
  else
    it->second->logExpiredInterest(pitEntry);
}

void SAFEngine::logNack(const Face& inFace, const Interest& interest)
{
  //log the nack
  std::string prefix = extractContentPrefix(interest.getName());
  SAFEntryMap::iterator it = entryMap.find (prefix);
  if(it == entryMap.end ())
    fprintf(stderr,"Error in SAFEntryLookUp\n");
  else
    it->second->logNack(inFace, interest);

  //return the token?
  SAFEntryMap::iterator i = entryMap.find (prefix);
  if(i == entryMap.end ())
    fprintf(stderr,"Error in SAFEntryLookUp\n");
  else
    return fbMap[inFace.getId ()]->receivedNack(prefix);
}

void SAFEngine::logRejectedInterest(shared_ptr<pit::Entry> pitEntry, int face_id)
{
  std::string prefix = extractContentPrefix(pitEntry->getName());
  SAFEntryMap::iterator it = entryMap.find (prefix);
  if(it == entryMap.end ())
    fprintf(stderr,"Error in SAFEntryLookUp\n");
  else
    it->second->logRejectedInterest(pitEntry, face_id);
}

std::string SAFEngine::extractContentPrefix(nfd::Name name)
{
  //fprintf(stderr, "extracting from %s\n", name.toUri ().c_str ());

  std::string prefix = "";
  for(int i=0; i <= prefixComponentNumber; i++)
  {
    prefix.append ("/");
    prefix.append (name.get (i).toUri ());
  }
  return prefix;
}

void SAFEngine::determineNodeName(const nfd::FaceTable& table)
{
  for(nfd::FaceTable::const_iterator it = table.begin (); it != table.end (); ++it)
  {
    if(ns3::ndn::NetDeviceFace* netf = dynamic_cast<ns3::ndn::NetDeviceFace*>(&(*(*it))))
    {
      nodeName = ns3::Names::FindName(netf->GetNetDevice()->GetNode());
      return;
    }
  }
  nodeName = "UnknownNode";
}
