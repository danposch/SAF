#include "parameterconfiguration.h"

ParameterConfiguration* ParameterConfiguration::instance = NULL;

ParameterConfiguration::ParameterConfiguration()
{
  setParameter ("LAMBDA", P_LAMBDA);
  setParameter ("UPDATE_INTERVALL", P_UPDATE_INTERVALL);
  setParameter ("MAX_LAYERS", P_MAX_LAYERS);
  setParameter ("DROP_FACE_ID", P_DROP_FACE_ID);
  setParameter ("RELIABILITY_THRESHOLD_MIN", P_RELIABILITY_THRESHOLD_MIN);
  setParameter ("RELIABILITY_THRESHOLD_MAX", P_RELIABILITY_THRESHOLD_MAX);
  setParameter ("HISTORY_SIZE",P_HISTORY_SIZE);
  setParameter ("CONTENT_AWARE_ADAPTATION", P_CONTENT_AWARE_ADAPTATION);
  setParameter ("PREFIX_COMPONENT", P_PREFIX_COMPONENT);
  setParameter ("RTX_DETECTION", P_USE_RTX_DETECTION);
}


void ParameterConfiguration::setParameter(std::string para_name, double value)
{
  pmap[para_name] = value;
}

double ParameterConfiguration::getParameter(std::string para_name)
{
  return pmap[para_name];
}

ParameterConfiguration *ParameterConfiguration::getInstance()
{
  if(instance == NULL)
    instance = new ParameterConfiguration();

  return instance;
}
