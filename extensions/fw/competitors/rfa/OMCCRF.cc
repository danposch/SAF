#include "OMCCRF.h"

using namespace nfd;
using namespace nfd::fw;

const Name OMCCRF::STRATEGY_NAME("ndn:/localhost/nfd/strategy/omccrf");

OMCCRF::OMCCRF(Forwarder &forwarder, const Name &name) : Strategy(forwarder, name)
{
  prefixComponents = ParameterConfiguration::getInstance ()->getParameter ("PREFIX_COMPONENT");
}

OMCCRF::~OMCCRF()
{
}

void OMCCRF::afterReceiveInterest(const Face& inFace, const Interest& interest ,shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
{
  /* Attention!!! interest != pitEntry->interest*/ // necessary to emulate NACKs in ndnSIM2.0
  /* interst could be /NACK/suffix, while pitEntry->getInterest is /suffix */

  if(!fibEntry->hasNextHops()) // check if nexthop(s) exist(s)
  {
    fprintf(stderr, "No next hop for prefix!\n");
    //rejectPendingInterest(pitEntry); this would create a nack we dont need it so just return
    return;
  }

  if(pitEntry->hasUnexpiredOutRecords()) //possible rtx or just the same request from a "different" source
    {
      if(!isRtx(inFace, interest))
      {
        addToKnownInFaces(inFace, interest); // other client/node requests same content
        return; // this aggregates the interest
      }
      //else just continue as it was a normal interest..
    }
  addToKnownInFaces(inFace, interest);

  std::string prefix = extractContentPrefix(pitEntry->getInterest().getName());

  if(pmap.find (prefix) == pmap.end ()) //check if prefix is listed if not create it
  {
    //create new entry
    pmap[prefix] = FacePicEntryMap();

    //add all new hops
    std::vector<fib::NextHop> nhops = fibEntry->getNextHops ();
    for(unsigned int i = 0; i < nhops.size(); i++)
    {
      pmap[prefix][nhops.at (i).getFace()->getId()] = boost::shared_ptr<PIC>(new PIC());
    }
  }

  // now prepare everything for inverse transform sampling to choose the outgoing face

  //calc sum for normalization and store weights in map
  std::vector<int> inFaces = getAllInFaces(pitEntry); // in faces are not considerd as outgoing face
  std::map<int /*face_id*/, double /*weight*/> wmap;
  double sum = 0.0;

  for(FacePicEntryMap::iterator it = pmap[prefix].begin(); it != pmap[prefix].end(); it++)
  {
    if(std::find(inFaces.begin (), inFaces.end (), it->first) == inFaces.end ()) // add only if not a inface
    {
      wmap[it->first]=it->second->getWeight();
      sum += it->second->getWeight();
    }
  }

  //normalize values in wMap
  for(std::map<int, double>::iterator k = wmap.begin (); k!=wmap.end (); k++)
  {
    wmap[k->first] = (k->second/sum);
  }

  //now draw a random number and choose outgoing face
  double rvalue = randomVariable.GetValue ();
  sum = 0.0;
  int out_face_id = -1;
  for(std::map<int, double>::iterator k = wmap.begin (); k!=wmap.end (); k++)
  {
    sum += k->second;
    if(rvalue <= sum)
    {
      out_face_id = k->first;
      break;
    }
  }

  if(out_face_id == -1) // no suitable face found
  {
    //rejectPendingInterest(pitEntry); this would create a nack we dont need it so just return
    return;
  }

  //entry must exist no need to use find method
  pmap[prefix][out_face_id]->increase();
  pmap[prefix][out_face_id]->update();

  sendInterest(pitEntry, getFaceTable ().get (out_face_id));
}

void OMCCRF::beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  if(inFace.getId () <= 255) // this are virtual faces like the content store/ skip it
  {
    Strategy::beforeSatisfyInterest (pitEntry, inFace, data);
    return;
  }

  boost::shared_ptr<PIC> p = findPICEntry(inFace.getId (), extractContentPrefix (data.getName ()));

  if(p != NULL)
  {
    p->decrease ();
    p->update ();
  }
  clearKnownFaces(pitEntry->getInterest());
  Strategy::beforeSatisfyInterest (pitEntry,inFace, data);
}

void OMCCRF::beforeExpirePendingInterest(shared_ptr< pit::Entry > pitEntry)
{

  std::vector<int> faces = getAllOutFaces (pitEntry);

  for(unsigned int i = 0; i < faces.size (); i++)
  {
    boost::shared_ptr<PIC> p = findPICEntry((*faces.begin ()), extractContentPrefix (pitEntry->getName ()));
    if(p != NULL)
    {
      p->decrease ();
      p->update ();
    }
  }
  clearKnownFaces(pitEntry->getInterest());
  Strategy::beforeExpirePendingInterest (pitEntry);
}

boost::shared_ptr<PIC> OMCCRF::findPICEntry(int face_id, std::string prefix)
{
  PrefixMap::iterator it = pmap.find (prefix);

  if(it == pmap.end())
  {
    fprintf(stderr, "Error could not find prefix in pmap!\n");
    return NULL;
  }

  FacePicEntryMap::iterator k = pmap[prefix].find(face_id);

  if(k == pmap[prefix].end())
  {
    fprintf(stderr, "Error could not find PICEntry for face\n");
    return NULL;
  }

  return pmap[prefix][face_id];
}


std::string OMCCRF::extractContentPrefix(nfd::Name name)
{
  std::string prefix = "";
  for(int i=0; i <= prefixComponents; i++)
  {
    prefix.append ("/");
    prefix.append (name.get (i).toUri ());
  }
  return prefix;
}

std::vector<int> OMCCRF::getAllOutFaces(shared_ptr<pit::Entry> pitEntry)
{
  std::vector<int> faces;
  const nfd::pit::OutRecordCollection records = pitEntry->getOutRecords();

  for(nfd::pit::OutRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
    faces.push_back((*it).getFace()->getId());

  return faces;
}

std::vector<int> OMCCRF::getAllInFaces(shared_ptr<pit::Entry> pitEntry)
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

bool OMCCRF::isRtx (const nfd::Face& inFace, const ndn::Interest& interest)
{
  KnownInFaceMap::iterator it = inFaceMap.find (interest.getName ().toUri());
  if(it == inFaceMap.end ())
    return false;

  for(std::list<int>::iterator k = it->second.begin(); k != it->second.end(); k++)
  {
    if((*k) == inFace.getId ())
      return true;
  }

  return false;
}

void OMCCRF::addToKnownInFaces(const nfd::Face& inFace, const ndn::Interest&interest)
{
  KnownInFaceMap::iterator it = inFaceMap.find (interest.getName ().toUri());

  if(it == inFaceMap.end ())
    inFaceMap[interest.getName ().toUri ()] = std::list<int>();

  std::list<int> list = inFaceMap[interest.getName ().toUri ()];

  if(std::find(list.begin (),list.end (), inFace.getId()) == list.end ()) //if face not known as in face
    inFaceMap[interest.getName ().toUri ()].push_back(inFace.getId());    //remember it
}

void OMCCRF::clearKnownFaces(const ndn::Interest&interest)
{
  KnownInFaceMap::iterator it = inFaceMap.find (interest.getName ().toUri());

  if(it == inFaceMap.end ())
  {
    //This may happen due to bad behavior of NDN core code!
    //beforeSatisfyInterest may be called multiple times for 1 pit entry..
    return;
  }
  inFaceMap.erase (it);
}

