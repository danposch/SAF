#include "oracle.h"

using namespace nfd;
using namespace nfd::fw;

const Name Oracle::STRATEGY_NAME("ndn:/localhost/nfd/strategy/oracle");

Oracle::Oracle(Forwarder &forwarder, const Name &name) : Strategy(forwarder, name)
{
  this->forwarder = &forwarder;

  //find the node we run on
  int counter = 0;
  ns3::Ptr<ns3::Node> tmp = StaticOracaleContainer::getInstance ()->getNode("StrategyNode"+boost::lexical_cast<std::string>(counter++));

  while(tmp != nullptr)
  {
    node = tmp;
    tmp = StaticOracaleContainer::getInstance ()->getNode("StrategyNode"+boost::lexical_cast<std::string>(counter++));
  }

  //register forwarder for node id;
  StaticOracaleContainer::getInstance ()->insertForwarder(node->GetId (),&forwarder);
}

Oracle::~Oracle()
{
}

void Oracle::afterReceiveInterest(const Face& inFace, const Interest& interest ,shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
{

  /* Attention!!! interest != pitEntry->interest*/ // necessary to emulate NACKs in ndnSIM2.0
  /* interst could be /NACK/suffix, while pitEntry->getInterest is /suffix */

  if(!fibEntry->hasNextHops())
    return;

  //get the shortest path (hops) to the content source
  int indexShortestHop = 0;
  int smallestHopCost = INT_MAX;
  std::vector< fib::NextHop > nextHops = fibEntry->getNextHops();

  for(int i = 0; i < nextHops.size (); i++)
  {
    if( ((int) nextHops.at (i).getCost()) < smallestHopCost)
    {
      indexShortestHop = i;
      smallestHopCost = ((int) nextHops.at (i).getCost());
    }
  }

  //find the shortest path towards a cache that is shorter than the path to content source
  std::vector<ns3::Ptr<ns3::Node> > visitedNodes;
  std::vector<ns3::ndn::NetDeviceFace* > inFaces = getAllInNetDeviceFaces (pitEntry);
  for(int i = 0; i < inFaces.size (); i++)
    visitedNodes.push_back (getCounterpart (inFaces.at (i), node));

  std::vector<ns3::Ptr<ns3::Node> > relevantNodes;
  relevantNodes.push_back (this->node);

  ns3::Ptr<ns3::Node> nr = findNearestReplica (visitedNodes, relevantNodes, pitEntry, smallestHopCost-1);

  if(nr == nullptr) // if we can not find a nearest replica
  {
    //just forward to the next hop indicated by the fib
    sendInterest(pitEntry, nextHops.at (indexShortestHop).getFace());
    return;
  }

  //else find the correct outgoing face for the node...
  shared_ptr<fib::Entry> fe = forwarder->getFib ().findLongestPrefixMatch ("/Node_"+boost::lexical_cast<std::string>(nr->GetId ()));

  nextHops = fe->getNextHops();
  indexShortestHop = 0;
  smallestHopCost = INT_MAX;
  for(int i = 0; i < nextHops.size (); i++)
  {
    if( ((int) nextHops.at (i).getCost()) < smallestHopCost)
    {
      indexShortestHop = i;
      smallestHopCost = ((int) nextHops.at (i).getCost());
    }
  }

  sendInterest(pitEntry, nextHops.at (indexShortestHop).getFace());
}

ns3::Ptr<ns3::Node> Oracle::findNearestReplica(std::vector<ns3::Ptr<ns3::Node> > visitedNodes,
                                std::vector<ns3::Ptr<ns3::Node> > relevantNodes, shared_ptr<pit::Entry> pitEntry, int remainingSteps)
{
  //add nodes that will be investigated to already visited nodes
  visitedNodes.insert(visitedNodes.end (), relevantNodes.begin (), relevantNodes.end ());
  std::vector<ns3::Ptr<ns3::Node> > newRelevantNodes; // new relevent hops for iteration i+1
  std::vector<ns3::Ptr<ns3::Node> > cacheHits; // nodes with a chache hit

  for(int i = 0; i < relevantNodes.size (); i++) //check all relevant nodes
  {
    std::vector<ns3::Ptr<ns3::NetDevice> > devices = getAllNetDevicesFromNode(relevantNodes.at (i));

    for(int k = 0; k < devices.size (); k++) // get all devices
    {
      ns3::Ptr<ns3::Node> counterPart = getCounterpart (devices.at (k), relevantNodes.at (i)); // get the counterpart node
      //check if counterPart has been visited

      if(std::find(visitedNodes.begin (), visitedNodes.end (), counterPart) == visitedNodes.end ()) // check if already visited
      {
        //if not was node visited check cache
        if(checkCacheHit(pitEntry,counterPart)) //check if node can satisfy interet...
          cacheHits.push_back (counterPart); //hit
        else //object not present save node for next iteration
        {
          if(std::find(newRelevantNodes.begin (), newRelevantNodes.end (), counterPart) == newRelevantNodes.end ())
            newRelevantNodes.push_back (counterPart);
        }
      }
    }
  }

  if(cacheHits.size () > 0) // we found at least one nearest replica
    return cacheHits.at (randomVariable.GetInteger (0, cacheHits.size()-1)); //return a random nr
  else if(remainingSteps > 0) // if we have steps left go to iteration i+1
    return findNearestReplica(visitedNodes, newRelevantNodes, pitEntry, remainingSteps-1);

  return nullptr; //no steps left, no nearst replica return nullptr.
}

void Oracle::beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  Strategy::beforeSatisfyInterest (pitEntry,inFace, data);
}

void Oracle::beforeExpirePendingInterest(shared_ptr< pit::Entry > pitEntry)
{
  Strategy::beforeExpirePendingInterest (pitEntry);
}

std::vector<ns3::ndn::NetDeviceFace* > Oracle::getAllInNetDeviceFaces(shared_ptr<pit::Entry> pitEntry)
{
  std::vector<ns3::ndn::NetDeviceFace* > faces;
  const nfd::pit::InRecordCollection records = pitEntry->getInRecords();

  for(nfd::pit::InRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
  {
    if(ns3::ndn::NetDeviceFace* netf = dynamic_cast<ns3::ndn::NetDeviceFace* >(&(*(*it).getFace())))
      faces.push_back(netf);
  }
  return faces;
}

std::vector<ns3::Ptr<ns3::NetDevice> > Oracle::getAllNetDevicesFromNode(ns3::Ptr<ns3::Node> node)
{
  std::vector<ns3::Ptr<ns3::NetDevice> > devs;

  for(int i = 0; i < node->GetNDevices (); i++)
      devs.push_back (node->GetDevice (i));

  return devs;
}

ns3::Ptr<ns3::Node> Oracle::getCounterpart(ns3::Ptr<ns3::NetDevice> face, ns3::Ptr<ns3::Node> node)
{
  //we assume strict point to point communication here
  if(face->GetChannel()->GetDevice(0)->GetNode ()->GetId () != node->GetId ())
    return face->GetChannel()->GetDevice(0)->GetNode ();
  else
    return face->GetChannel()->GetDevice(1)->GetNode ();
}

ns3::Ptr<ns3::Node> Oracle::getCounterpart(ns3::ndn::NetDeviceFace* face, ns3::Ptr<ns3::Node> node)
{
  //we assume strict point to point communication here
  if(face->GetNetDevice ()->GetChannel()->GetDevice(0)->GetNode ()->GetId () != node->GetId ())
    return face->GetNetDevice ()->GetChannel()->GetDevice(0)->GetNode ();
  else
    return face->GetNetDevice ()->GetChannel()->GetDevice(1)->GetNode ();
}

bool Oracle::checkCacheHit(shared_ptr<pit::Entry> pitEntry, ns3::Ptr<ns3::Node> node)
{

  Interest interest = pitEntry->getInterest ();
  Forwarder* fw = StaticOracaleContainer::getInstance ()->getForwarder(node->GetId ());

  if(fw == nullptr) // node has not registerd himself, e.g., a node that does not run the oracle strategy...
    return false;

  ns3::Ptr<ns3::ndn::ContentStore> csFromNdnSim = fw->getCsFromNdnSim ();

  if (csFromNdnSim == nullptr)
  {
   const Data* csMatch = fw->getCs ().find(interest);

   if (csMatch != 0)
     return true;
  }
  else
  {
   shared_ptr<Data> csMatch = csFromNdnSim->Lookup(make_shared<Interest>(interest));
   if(csMatch != nullptr)
     return true;
  }

  return false;
}
