#ifndef EXTENDEDGLOBALROUTINGHELPER_H
#define EXTENDEDGLOBALROUTINGHELPER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndn-all.hpp"
#include "ns3/ndnSIM/helper/ndn-global-routing-helper.hpp"
#include "ns3/object.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/net-device.h"
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/names.h"
#include "ns3/node-list.h"
#include "ns3/channel-list.h"
#include "ns3/object-factory.h"

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"
#include "model/ndn-net-device-face.hpp"
#include "model/ndn-global-router.hpp"

#include "daemon/table/fib.hpp"
#include "daemon/fw/forwarder.hpp"
#include "daemon/table/fib-entry.hpp"
#include "daemon/table/fib-nexthop.hpp"

namespace ns3
{
namespace ndn
{

class ExtendedGlobalRoutingHelper : public ns3::ndn::GlobalRoutingHelper
{
public:
  ExtendedGlobalRoutingHelper();

  void AddOriginsForAllUsingNodeIds();
};

}
}
#endif // EXTENDEDGLOBALROUTINGHELPER_H
