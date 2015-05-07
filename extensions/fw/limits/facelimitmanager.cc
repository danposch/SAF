#include "facelimitmanager.h"

using namespace nfd;
using namespace nfd::fw;

FaceLimitManager::FaceLimitManager(shared_ptr< Face > face)
{
  this->face = face;

  double packets_per_sec = getPhysicalBitrate (face) / 8 ;
  packets_per_sec /= (DATA_PACKET_SIZE + INTEREST_PACKET_SIZE);
  tokenGenRate = packets_per_sec / 1000; // tokens per ms
  tokenGenRate *= TOKEN_FILL_INTERVALL; // tokens per intervall

  //fprintf(stderr, "packets_per_sec %f\n", packets_per_sec );
  //fprintf(stderr, "tokenGenRate %f\n", tokenGenRate );

  this->newTokenEvent = ns3::Simulator::Schedule(ns3::Seconds(0), &FaceLimitManager::newToken, this);
}

bool FaceLimitManager::addNewPrefix(std::string content_prefix)
{
  //use the basic limiter for now
  bMap[content_prefix] = boost::shared_ptr<Limiter>(new Limiter(std::max(tokenGenRate*5.0, 2.0))); // for now we give all tokenbuckets a const size we should adapt this later
  //bMap[content_prefix] = boost::shared_ptr<Limiter>(new Limiter(BUCKET_SIZE)); // for now we give all tokenbuckets a const size we should adapt this later
}

void  FaceLimitManager::newToken()
{
  double rest = tokenGenRate;

  std::vector<std::string> indizes = getAllNonFullBuckets();

  while(indizes.size () != 0 && rest != 0)
  {
    double tokens = rest;
    rest = 0;

    for(std::vector<std::string>::iterator it = indizes.begin (); it != indizes.end (); ++it)
    {
      rest += bMap[*it]->addTokens(tokens / (double) indizes.size ());
      //fprintf(stderr, "added %f tokens to bucket %s\n", tokens / (double) indizes.size (), (*it).c_str());
    }
    indizes = getAllNonFullBuckets ();
  }
  this->newTokenEvent = ns3::Simulator::Schedule(ns3::MilliSeconds(TOKEN_FILL_INTERVALL), &FaceLimitManager::newToken, this);
}

uint64_t FaceLimitManager::getPhysicalBitrate(shared_ptr< Face > face)
{
  if(ns3::ndn::NetDeviceFace *netf = dynamic_cast<ns3::ndn::NetDeviceFace*>(&(*face)))
  {
    /*ns3::Ptr<ns3::Node> node = netf->GetNetDevice()->GetNode ();
    fprintf(stderr, "faceId = %d\n", netf->getId ());
    for(int i = 0; i < node->GetNDevices (); i++)
    {
      fprintf(stderr, "Device[%d] found\n", i);
    }
    fprintf(stderr, "\n");*/
    //-256 because 0-255 is resevred by ndn local faces;
    ns3::Ptr<ns3::PointToPointNetDevice> nd1 = netf->GetNetDevice()->GetNode ()->GetDevice(netf->getId () - 256)->GetObject<ns3::PointToPointNetDevice>();
    ns3::DataRateValue dv;
    nd1->GetAttribute("DataRate", dv);
    ns3::DataRate d = dv.Get();
    //fprintf(stderr, "bitrate on face = %llu\n", d.GetBitRate());
    return d.GetBitRate();
  }
  else
  {
    return ULONG_MAX;
  }
}

std::vector<std::string> FaceLimitManager::getAllNonFullBuckets()
{
  std::vector<std::string> vec;
  for(LimitMap::iterator it = bMap.begin (); it != bMap.end (); ++it)
  {
    if(!it->second->isFull())
      vec.push_back(it->first);
  }
  return vec;
}

bool FaceLimitManager::tryForwardInterest(std::string prefix)
{
  return bMap[prefix]->tryConsumeToken();
}

void FaceLimitManager::receivedNack(std::string prefix)
{
  bMap[prefix]->addTokens(1.0 * NACK_RETURN_TOKEN);
}
