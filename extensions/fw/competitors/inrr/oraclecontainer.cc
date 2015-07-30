#include "oraclecontainer.h"

using namespace nfd;
using namespace nfd::fw;

StaticOracaleContainer* StaticOracaleContainer::instance = NULL;

StaticOracaleContainer::StaticOracaleContainer()
{

}

StaticOracaleContainer *StaticOracaleContainer::getInstance()
{
  if(instance == NULL)
    instance = new StaticOracaleContainer();

  return instance;
}

void StaticOracaleContainer::insertNode(const std::string& name, ns3::Ptr<ns3::Node> n)
{
  if(m.find (name) == m.end ())
    m[name] = n;
  else
    fprintf(stderr, "Error Name is already taken\n");
}

ns3::Ptr<ns3::Node> StaticOracaleContainer::getNode(const std::string& name)
{
  if(m.find (name) != m.end ())
    return m[name];

  return nullptr;
}

void StaticOracaleContainer::insertForwarder(int node_id, Forwarder *forwarder)
{
  if(fw_map.find (node_id) == fw_map.end ())
    fw_map[node_id] = forwarder;
  else
    fprintf(stderr, "Forwarder already registered\n");
}

nfd::Forwarder* StaticOracaleContainer::getForwarder(int node_id)
{
  if(fw_map.find (node_id) != fw_map.end ())
    return fw_map[node_id];

  return nullptr;
}
