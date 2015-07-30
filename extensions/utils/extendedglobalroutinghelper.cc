#include "extendedglobalroutinghelper.h"

using namespace ns3::ndn;

ExtendedGlobalRoutingHelper::ExtendedGlobalRoutingHelper() : GlobalRoutingHelper()
{
}

void ExtendedGlobalRoutingHelper::AddOriginsForAllUsingNodeIds()
{
  for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++)
  {
    Ptr<GlobalRouter> gr = (*node)->GetObject<GlobalRouter>();
    if (gr != 0)
    {
      AddOrigin("/Node_" + boost::lexical_cast<std::string>((*node)->GetId()), *node);
    }
  }
}
