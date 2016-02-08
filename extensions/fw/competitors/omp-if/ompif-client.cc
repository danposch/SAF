#include "ompif-client.h"

using namespace nfd;
using namespace nfd::fw;

const Name OMPIFClient::STRATEGY_NAME("ndn:/localhost/nfd/strategy/ompif");

OMPIFClient::OMPIFClient(Forwarder &forwarder, const Name &name) : OMPIF(forwarder, name)
{
  type = OMPIFType::Client;
}
