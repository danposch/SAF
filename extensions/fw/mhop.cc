#include "mhop.h"
#include "climits"
#include <iostream>
#include "../utils/parameterconfiguration.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.hpp"
#include "ns3/ndnSIM/utils/ndn-ns3-packet-tag.hpp"

namespace nfd {
namespace fw {

MHop::MHop(std::vector<int> faces, int max_hops) : Mratio(faces)
{
  this->type = MeasureType::MHop;
  curMaxHop = max_hops;
}

void MHop::logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{

  int hopCount = 0;

  auto ns3PacketTag = data.getTag<ns3::ndn::Ns3PacketTag>();

  ns3::ndn::FwHopCountTag hopCountTag;
  if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag))
    hopCount = hopCountTag.Get();

  if (hopCount > curMaxHop)
    Mratio::logExpiredInterest(pitEntry);
  else
    Mratio::logSatisfiedInterest(pitEntry,inFace, data);
}

}
}


