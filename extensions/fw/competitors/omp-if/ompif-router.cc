#include "ompif-router.h"

using namespace nfd;
using namespace nfd::fw;

const Name OMPIFRouter::STRATEGY_NAME("ndn:/localhost/nfd/strategy/ompif-router");

OMPIFRouter::OMPIFRouter(Forwarder &forwarder, const Name &name) : OMPIF(forwarder, name)
{
  type = OMPIFType::Router;
}
