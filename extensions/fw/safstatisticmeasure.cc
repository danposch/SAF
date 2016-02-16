#include "safstatisticmeasure.h"

using namespace nfd;
using namespace nfd::fw;

NS_LOG_COMPONENT_DEFINE("SAFStatisticMeasure");

SAFStatisticMeasure::SAFStatisticMeasure(std::vector<int> faces)
{
  this->type = MeasureType::UNKOWN;
  this->faces = faces;

  // initalize
  for(int layer=0; layer < (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS"); layer ++) // for each layer
  {
    for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each face
    {
      stats[layer].unsatisfied_requests[*it] = 0;
      stats[layer].satisfied_requests[*it] = 0;

      stats[layer].last_reliability[*it] = 0;
      stats[layer].last_actual_forwarding_probs[*it] = 0;
      stats[layer].satisfaction_variance[*it] = INIT_VARIANCE;// a high value
      stats[layer].last_unsatisfied_requests[*it] = 0;
      stats[layer].last_satisfied_requests[*it] = 0;
      stats[layer].ema_alpha[*it] = 0.0;
    }
  }
}

SAFStatisticMeasure::~SAFStatisticMeasure()
{
}

void SAFStatisticMeasure::update (std::map<int,double> reliability_t)
{
  for(int layer=0; layer < (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS"); layer ++) // for each layer
  {
    // clear old computed values
    stats[layer].last_reliability.clear();
    stats[layer].last_actual_forwarding_probs.clear();

    //calculate new values
    calculateTotalForwardedRequests(layer);
    calculateLinkReliabilities (layer, reliability_t[layer]);
    calculateActualForwardingProbabilities (layer);
    updateVariance (layer);
    calculateEMAAlpha(layer);

    //copy some old values before clearing
    stats[layer].last_unsatisfied_requests = stats[layer].unsatisfied_requests;
    stats[layer].last_satisfied_requests = stats[layer].satisfied_requests;

    //clear collected information
    stats[layer].unsatisfied_requests.clear ();
    stats[layer].satisfied_requests.clear ();

    //initialize for next period
    for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each face
    {
      stats[layer].unsatisfied_requests[*it] = 0;
      stats[layer].satisfied_requests[*it] = 0;
    }
  }
}

void SAFStatisticMeasure::calculateTotalForwardedRequests(int layer)
{
  stats[layer].total_forwarded_requests = 0;
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
  {
    stats[layer].total_forwarded_requests += stats[layer].unsatisfied_requests[*it] + stats[layer].satisfied_requests[*it];
  }
}

void SAFStatisticMeasure::calculateLinkReliabilities(int layer, double reliability_t)
{
  //NS_LOG_DEBUG("Calculating link reliability for layer " << layer <<": \t threshold:" << reliability_t);
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each face
  {
    if(stats[layer].unsatisfied_requests[*it] == 0)
      stats[layer].last_reliability[*it] = 1.0;
    else
      stats[layer].last_reliability[*it] =
          (double)stats[layer].satisfied_requests[*it] / ((double)(stats[layer].unsatisfied_requests[*it] + stats[layer].satisfied_requests[*it]));

    /*NS_LOG_DEBUG("Reliabilty Face(" << *it << ")=" << stats[layer].last_reliability[*it] << "\tin total "
        << stats[layer].unsatisfied_requests[*it] + stats[layer].satisfied_requests[*it] << " interest forwarded");*/
  }
}

void SAFStatisticMeasure::calculateActualForwardingProbabilities (int layer)
{
  double sum = stats[layer].total_forwarded_requests;

  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
  {
    if(sum == 0)
      stats[layer].last_actual_forwarding_probs[*it] = 0;
    else
      stats[layer].last_actual_forwarding_probs[*it] =
        (stats[layer].unsatisfied_requests[*it] + stats[layer].satisfied_requests[*it]) / sum;
  }
}

std::vector<int> SAFStatisticMeasure::getReliableFaces(int layer, double reliability_t)
{
  std::vector<int> reliable;
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
  {
    if(*it == DROP_FACE_ID)
      continue;

    if(getFaceReliability(*it, layer) >= reliability_t)
      reliable.push_back (*it);
  }
  return reliable;
}

std::vector<int>  SAFStatisticMeasure::getUnreliableFaces(int layer, double reliability_t)
{
  std::vector<int> unreliable;
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
  {
    if(*it == DROP_FACE_ID)
      continue;

    if(getFaceReliability(*it, layer) < reliability_t)
      unreliable.push_back (*it);
  }
  return unreliable;
}

double SAFStatisticMeasure::getFaceReliability(int face_id, int layer)
{
  return stats[layer].last_reliability[face_id];
}

int SAFStatisticMeasure::determineContentLayer(const Interest& interest)
{
  //TODO once content layers are introduced

  /*std::string layer = interest.getName ().get (1).toUri ();
  //fprintf(stderr, "layer = %s\n",interest.getName ().get (1).toUri ().c_str ());

  if(layer.compare ("layer0") == 0)
    return 0;

  if(layer.compare ("layer1") == 0)
    return 1;

  if(layer.compare ("layer2") == 0)
    return 2;

  layer = interest.getName ().toUri ();

  if(layer.find ("-L0.svc") != std::string::npos)
    return 0;

  if(layer.find ("-L1.svc") != std::string::npos)
    return 1;

  if(layer.find ("-L2.svc") != std::string::npos)
    return 2;*/

  return 0;
}

void SAFStatisticMeasure::updateVariance (int layer)
{
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each face
  {
    stats[layer].satisfied_requests_history[*it].push_back(stats[layer].satisfied_requests[*it]);
    if(stats[layer].satisfied_requests_history[*it].size() > ParameterConfiguration::getInstance ()->getParameter("HISTORY_SIZE"))
      stats[layer].satisfied_requests_history[*it].pop_front();

    if(stats[layer].satisfied_requests_history[*it].size() <= 1)
    {
      stats[layer].satisfaction_variance[*it] = INIT_VARIANCE;
    }
    else
    {
      double avg = 0.0;
      for(std::list<int>::iterator lit = stats[layer].satisfied_requests_history[*it].begin();
          lit != stats[layer].satisfied_requests_history[*it].end(); ++lit)
      {
        //fprintf(stderr, "lit=%d\n", *lit);
        avg += (*lit);
      }
      avg = avg/stats[layer].satisfied_requests_history[*it].size();

      stats[layer].satisfaction_variance[*it] = 0.0;
      for(std::list<int>::iterator lit = stats[layer].satisfied_requests_history[*it].begin();
          lit != stats[layer].satisfied_requests_history[*it].end(); ++lit)
      {
        stats[layer].satisfaction_variance[*it] += pow((double(*lit)) - avg,2);
      }
      stats[layer].satisfaction_variance[*it] /= stats[layer].satisfied_requests_history[*it].size();
    }
    //fprintf(stderr, "variance=%f\n",stats[layer].satisfaction_variance[*it]);
  }
  //fprintf(stderr, "\n");
}

void SAFStatisticMeasure::calculateEMAAlpha(int layer)
{
  double w = 0.2;
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each face
  {
    stats[layer].ema_alpha[*it] = w * getAlpha(*it,layer) + (1-w)*stats[layer].ema_alpha[*it];
  }
}

double SAFStatisticMeasure::getAlpha(int face_id, int layer)
{
  //return 1.0/(1.0 + stats[layer].satisfaction_variance[face_id]);
  return 1.0/(1.0 + std::sqrt(stats[layer].satisfaction_variance[face_id]));
}

double SAFStatisticMeasure::getEMAAlpha(int face_id, int layer)
{
  return stats[layer].ema_alpha[face_id];
}

double SAFStatisticMeasure::getRho(int layer)
{
  if(stats[layer].total_forwarded_requests == 0)
    return 0.0;

  double sum_satisfied = 0.0;
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each face
  {
    if(*it == DROP_FACE_ID)
      continue;
    //fprintf(stderr, "Satisfied on face[%d]=%d\n", *it, stats[layer].last_satisfied_requests[*it]);
    sum_satisfied += stats[layer].last_satisfied_requests[*it];
  }
  //fprintf(stderr, "sum_satisfied = %f; total_forwarded=%d\n", sum_satisfied, stats[layer].total_forwarded_requests);
  return 1.0 - (sum_satisfied / stats[layer].total_forwarded_requests);
}

void SAFStatisticMeasure::addFace(shared_ptr<Face> face)
{

  int face_id = face->getId();

  for(int layer=0; layer < (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS"); layer ++) // for each layer
  {
    //init stats for face
    stats[layer].unsatisfied_requests[face_id] = 0;
    stats[layer].satisfied_requests[face_id] = 0;

    stats[layer].last_reliability[face_id] = 0;
    stats[layer].last_actual_forwarding_probs[face_id] = 0;
    stats[layer].satisfaction_variance[face_id] = INIT_VARIANCE;// a high value
    stats[layer].last_unsatisfied_requests[face_id] = 0;
    stats[layer].last_satisfied_requests[face_id] = 0;
    stats[layer].ema_alpha[face_id] = 0.0;
  }

  // push face back
  faces.push_back (face_id);

}

void SAFStatisticMeasure::removeFace(shared_ptr<Face> face)
{

  int id = face->getId();
  //remove face
  faces.erase(std::find(faces.begin (), faces.end (), id));

  for(int layer=0; layer < (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS"); layer ++) // for each layer
  {
    //init stats for face
    stats[layer].unsatisfied_requests.erase(id);
    stats[layer].satisfied_requests.erase(id);

    stats[layer].last_reliability.erase(id);
    stats[layer].last_actual_forwarding_probs.erase(id);
    stats[layer].satisfaction_variance.erase(id);
    stats[layer].last_unsatisfied_requests.erase(id);
    stats[layer].last_satisfied_requests.erase(id);
    stats[layer].ema_alpha.erase(id);
  }
}
