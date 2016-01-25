#include "safforwardingtable.h"

using namespace nfd;
using namespace nfd::fw;
using namespace boost::numeric::ublas;

NS_LOG_COMPONENT_DEFINE("SAFForwardingTable");

SAFForwardingTable::SAFForwardingTable(std::vector<int> faceIds, std::map<int, int> preferedFacesIds)
{
  for(int i = 0; i < (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS"); i++)
    curReliability[i]=ParameterConfiguration::getInstance ()->getParameter ("RELIABILITY_THRESHOLD_MAX");

  this->faces = faceIds;
  this->preferedFaces = preferedFacesIds;
  initTable ();
}

std::map<int, double> SAFForwardingTable::calcInitForwardingProb(std::map<int, int> preferedFacesIds, double gamma)
{
  std::map<int, double> res;
  double sum = 0;

  //use gamma to modify the weight.
  for(std::map<int,int>::iterator it = preferedFacesIds.begin (); it != preferedFacesIds.end (); it++)
    res[it->first] = pow((double)it->second, gamma);

  //calc the sum of the values
  for(std::map<int,double>::iterator it = res.begin (); it != res.end (); it++)
    sum += it->second;

  //divide each value by the sum
  for(std::map<int,double>::iterator it = res.begin (); it != res.end (); it++)
    res[it->first] = sum / (double) it->second;

  sum = 0;
  //calc the normalize factor
  for(std::map<int,double>::iterator it = res.begin (); it != res.end (); it++)
    sum += it->second;

  //normalize
  for(std::map<int,double>::iterator it = res.begin (); it != res.end (); it++)
    res[it->first] = it->second / sum;

  return res;
}

std::map<int, double> SAFForwardingTable::minHop(std::map<int, int> preferedFacesIds)
{
  std::map<int, double> res;

  int min_hop_face = 0;
  int min_hops = INT_MAX;

  for(std::map<int,int>::iterator it = preferedFacesIds.begin (); it != preferedFacesIds.end (); it++)
  {
    if(it->second < min_hops)
    {
      min_hop_face = it ->first;
      min_hops = it->second;
    }
  }
  res[min_hop_face] = 1.0;
  return res;
}

void SAFForwardingTable::initTable ()
{
  std::sort(faces.begin(), faces.end());//order

  table = matrix<double> (faces.size () /*rows*/, (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS") /*columns*/);

  std::map<int, double> initValues = calcInitForwardingProb (preferedFaces, 5.0);
  //std::map<int, double> initValues = minHop(preferedFaces);

  // fill matrix column-wise /* table(i,j) = i-th row, j-th column*/
  for (unsigned j = 0; j < table.size2 (); ++j) /* columns */
  {
    for (unsigned i = 0; i < table.size1 (); ++i) /* rows */
    {
      if(faces.at (i) == DROP_FACE_ID)
        table(i,j) = 0.0;
      else if(initValues.size ()== 0)
      {
        table(i,j) = (1.0 / ((double)faces.size () - 1.0)); /*set default value to 1 / (d - 1) */
      }
      else
      {
        std::map<int,double>::iterator it = initValues.find(faces.at (i));
        if( it != initValues.end ())// && it->first == faceId)
        {
          //fprintf(stderr, "Giving table(%d,%d)=%f\n", i,j,it->second);
          table(i,j) = it->second;
          //table(i,j) = (1.0 / ((double)preferedFaces.size ())); // preferedFaces dont include the dropping face.
          //table(i,j) = 1.0;
        }
        else
        {
          table(i,j) = 0;
        }
      }
    }
  }
}

int SAFForwardingTable::determineNextHop(const Interest& interest, std::vector<int> alreadyTriedFaces)
{
  //create a copy of the table
  matrix<double> tmp_matrix(table);
  std::vector<int> face_list(faces);

  int ilayer = SAFStatisticMeasure::determineContentLayer(interest);
  //lets check if sum(Fi in alreadyTriedFaces > R)
  double fw_prob = 0.0;
  int row = -1;
  for(std::vector<int>::iterator i = alreadyTriedFaces.begin (); i != alreadyTriedFaces.end ();++i)
  {
    row = determineRowOfFace(*i, tmp_matrix, face_list);
    if(row != FACE_NOT_FOUND)
    {
      fw_prob += tmp_matrix(row,ilayer);
      //fprintf(stderr, "face %d\n has been alread tried\n, i");
    }
    else
    {
      fprintf(stderr, "Could not find tried face[i]=%d. Returning the dropping face..\n", *i);
      return DROP_FACE_ID;
    }
  }

  if(fw_prob >= curReliability[ilayer] && fw_prob != 0) // in this case we drop...
  {
    return DROP_FACE_ID;
  }

  //ok now remove the alreadyTriedFaces
  int offset = 0;
  for(std::vector<int>::iterator i = alreadyTriedFaces.begin (); i != alreadyTriedFaces.end ();++i)
  {
    offset = determineRowOfFace(*i, tmp_matrix, face_list);
    if(offset != FACE_NOT_FOUND)
    {
      //then remove the row and the face from the list
      tmp_matrix = removeFaceFromTable(*i, tmp_matrix, face_list);
      face_list.erase (face_list.begin ()+offset);
    }
    else// if(*i != inFaces->GetId ())
    {
     fprintf(stderr, "Could not remove tired face[i]=%d. Returning the dropping face..\n", *i);
      return DROP_FACE_ID;
    }
  }

  // choose one face as outgoing according to the probability
  return chooseFaceAccordingProbability(tmp_matrix, ilayer, face_list);
}

void SAFForwardingTable::update(boost::shared_ptr<SAFStatisticMeasure> stats)
{
  std::vector<int> r_faces;  /*reliable faces*/
  std::vector<int> ur_faces; /*unreliable faces*/
  std::vector<int> p_faces;  /*probing faces*/

  NS_LOG_DEBUG("FWT Before Update:\n" << table); /* prints matrix line by line ( (first line), (second line) )*/

  for(int layer = 0; layer < (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS"); layer++) // for each layer
  {
    NS_LOG_DEBUG("Updating Layer[" << layer << "] with reliability_t=" << curReliability[layer]);

    //determine the set of (un)reliable faces
    r_faces = stats->getReliableFaces (layer, curReliability[layer]);
    ur_faces = stats->getUnreliableFaces (layer, curReliability[layer]);

    //seperate reliable faces from probing faces
    for(std::vector<int>::iterator it = r_faces.begin(); it != r_faces.end();)
    {
      if(stats->getForwardedInterests (*it, layer) == 0)
      {
        p_faces.push_back (*it);
        r_faces.erase (it);
      }
      else
        ++it;
    }

    for(std::vector<int>::iterator it = r_faces.begin(); it != r_faces.end(); ++it)
      NS_LOG_DEBUG("Reliable Face[" << *it << "]=" << stats->getFaceReliability(*it,layer)
                   << "\t "<< stats->getForwardedInterests (*it,layer) << " interest forwarded");
    for(std::vector<int>::iterator it = ur_faces.begin(); it != ur_faces.end(); ++it)
      NS_LOG_DEBUG("Unreliable Face[" << *it << "]=" << stats->getFaceReliability(*it,layer)
                   << "\t "<< stats->getForwardedInterests (*it,layer) << " interest forwarded");
    for(std::vector<int>::iterator it = p_faces.begin(); it != p_faces.end(); ++it)
      NS_LOG_DEBUG("Probe Face[" << *it << "]=" << stats->getFaceReliability(*it,layer)
                   << "\t "<< stats->getForwardedInterests (*it,layer) << " interest forwarded");
    NS_LOG_DEBUG("Drop Face[" << DROP_FACE_ID << "]=" << stats->getFaceReliability(DROP_FACE_ID,layer)
                 << "\t "<< stats->getForwardedInterests (DROP_FACE_ID,layer) << " interest forwarded");

    // ok treat the unreliable faces first...
    double utf = table(determineRowOfFace (DROP_FACE_ID),layer);
    double utf_face = 0.0;
    for(std::vector<int>::iterator it = ur_faces.begin (); it != ur_faces.end (); it++)
    {
      utf_face=stats->getAlpha (*it, layer) * stats->getUT(*it, layer);

      if(utf_face <= table(determineRowOfFace (*it),layer))
      {
        NS_LOG_DEBUG("Face[" << *it <<"]: Removing alpha[" << *it <<"]*UT[" << *it << "]="
                 << stats->getAlpha(*it, layer) << "*" << stats->getUT(*it, layer) << "=" << utf_face);
      }
      else
      {
        utf_face = table(determineRowOfFace (*it),layer);
        NS_LOG_DEBUG("Face[" << *it <<"]: Removing all =" << utf_face);
      }

      // remove traffic and store removed fraction
      table(determineRowOfFace (*it),layer) -= utf_face;
      utf += utf_face;
    }

    NS_LOG_DEBUG("Total UTF = " << utf);

    if(utf > 0)
    {
      if(r_faces.size () > 0)
      {
        // try to split the utf on r_faces
        std::map<int/*faceId*/, double /*traffic that can be shifted to face*/> ts;
        double ts_sum = 0.0;
        for(std::vector<int>::iterator it = r_faces.begin(); it != r_faces.end(); ++it) // for each r_face
        {
          NS_LOG_DEBUG("Face[" << *it <<"]: getS() / curReliability[*it] = "
                       << ((double)stats->getS (*it, layer)) << "/" << curReliability[layer]);
          ts[*it] = ((double)stats->getS (*it, layer)) / curReliability[layer];
          ts[*it] -= stats->getForwardedInterests (*it, layer);
          ts_sum += ts[*it];
          NS_LOG_DEBUG("Face[" << *it <<"]: still can take " <<  ts[*it] << " more Interests");
        }
        NS_LOG_DEBUG("Total Interests that can be taken by F_R = " << ts_sum);

        // find the minium fraction that can be AND should be shifted
        double min_fraction = (ts_sum / (double) stats->getTotalForwardedInterests (layer));
        if(min_fraction > utf)
          min_fraction = utf;

        NS_LOG_DEBUG("Total fraction that will be shifted to F_R= " << min_fraction);

        //now shift traffic to r_faces
        for(std::vector<int>::iterator it = r_faces.begin(); it != r_faces.end(); ++it) // for each r_face
        {
          NS_LOG_DEBUG("Face[" << *it <<"]: Adding (min_fraction*ts[" << *it << "]) / (ts_sum)="
                     << "(" << min_fraction << "*" << ts[*it] << ") / (" <<
                     ts_sum << ")=" << (min_fraction * ts[*it]) / ts_sum );

          table(determineRowOfFace (*it),layer) += (min_fraction * ts[*it]) / (ts_sum);
        }

        utf -= min_fraction; //remove the shifted fraction from the utf
      }

      //set the remaining utf to the dropping face
      NS_LOG_DEBUG("UTF remaining for the Dropping Face = "<< utf);
      table(determineRowOfFace (DROP_FACE_ID), layer) = utf;

      //check if probing could be done
      if(table(determineRowOfFace (DROP_FACE_ID), layer) > 0)
      {
        probeColumn (p_faces, layer, stats);

        if(table(determineRowOfFace (DROP_FACE_ID), layer) > (1-curReliability[layer]))
          decreaseReliabilityThreshold (layer);
      }
    }
    else if(stats->getTotalForwardedInterests (layer) > 0)
      increaseReliabilityThreshold (layer);

  }
  //finally just normalize to remove the rounding errors
  table = normalizeColumns(table);
  NS_LOG_DEBUG("FWT After Update:\n" << table); /* prints matrix line by line ( (first line), (second line) )*/
}

void SAFForwardingTable::probeColumn(std::vector<int> faces, int layer, boost::shared_ptr<SAFStatisticMeasure> stats)
{
  if(faces.size () == 0)
    return;

   //double probe = table(determineRowOfFace (DROP_FACE_ID), layer) * ParameterConfiguration::getInstance ()->getParameter ("PROBING_TRAFFIC");

  double lweight = (1.0/((double) (pow(1.0+(double)layer,2.0) - layer)));
  double probe = table(determineRowOfFace (DROP_FACE_ID), layer) * stats->getRho (layer) * lweight;
  NS_LOG_DEBUG("Probing! Probe Size = p(F_D) * rho * lweight= " << table(determineRowOfFace (DROP_FACE_ID), layer)
                 << "*" << stats->getRho (layer) << " * "<< lweight << "=" << probe);

  if(probe < 0.001) // if probe is zero return
    return;

  /*NS_LOG_DEBUG("Probing! Probe Size = p(F_D) * rho = " << table(determineRowOfFace (DROP_FACE_ID), layer)
               << "*" << ParameterConfiguration::getInstance ()->getParameter ("PROBING_TRAFFIC") << "=" << probe);*/
  NS_LOG_DEBUG("Probing! Probe Size = p(F_D) * rho = " << table(determineRowOfFace (DROP_FACE_ID), layer)
                 << "*" << stats->getRho (layer) << "=" << probe);

  //remove the probing traffic from F_D
  table(determineRowOfFace (DROP_FACE_ID), layer) -= probe;

  double normFactor = 0.0; // optimization for layers > 0
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
  {
    normFactor += table(determineRowOfFace (*it), 0);
  }

  //split the probe (forwarding probabilties)....
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
  {
    if(layer == 0 || normFactor == 0)
    {
      table(determineRowOfFace (*it), layer) += (probe / ((double)faces.size ()));
    }
    else
    {
      table(determineRowOfFace (*it), layer) += (probe * (table(determineRowOfFace (*it), 0) / normFactor));
    }
  }
}

void SAFForwardingTable::crossLayerAdaptation(boost::shared_ptr<SAFStatisticMeasure> smeasure)
{
  //investigate all layers for dropping traffic
  std::vector<int> adp_layers;
  for(int layer = 0; layer < (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS") - 1; layer++) // -1 as last layer can not be adapted anyway
  {
    //if under layer is under observation
    if(observed_layers.find (layer) != observed_layers.end ())
    {
      //check if steps are left
      if(observed_layers[layer] > 0)
        observed_layers[layer] -= 1; // reduce steps by 1
      else
      {
        adp_layers.push_back (layer);
        observed_layers.erase (observed_layers.find (layer));
      }
    }
    // check if we currently drop traffic for non observed layer
    else if(table(determineRowOfFace(DROP_FACE_ID),layer) > 0)
    {
      //if under investigation skip this layer
      if(observed_layers.find (layer) != observed_layers.end ())
        continue;

      //get the set of unreliable faces:
      double n = 0;
      double n_max = 0;
      double rel_t = curReliability[layer];
      double total_interests = smeasure->getTotalForwardedInterests (layer);
      double satisfied_interests = 0;
      double p0 = 0.0;
      double ema_alpha = 0.0;

      //ensure that I > 0
      if(total_interests == 0)
      {
        //observed_layers[layer] = MAX_OBSERVATION_PERIODS;
        //skip it for this perios and wait for next as we cant really say what do
        continue;
      }

      NS_LOG_DEBUG("Calculating number of periods to wait for layer " << layer << " to stabilize");
      std::vector<int> ur_faces = smeasure->getUnreliableFaces (layer, rel_t);
      for(std::vector<int>::iterator it = ur_faces.begin (); it != ur_faces.end (); ++it)
      {
        // ok calculate expected steps when F_i € F_U will be reliable
        p0 = table(determineRowOfFace (*it),layer);
        ema_alpha = smeasure->getEMAAlpha (*it,layer);
        satisfied_interests = smeasure->getS(*it,layer);

        //check if d(F_i) > 0
        if(satisfied_interests < 1)
        {
          //actually we are very likly that *it is a probing face, so just skip it
          NS_LOG_DEBUG( "n["<< *it << "] = skipped, \t We think it is a probing face");
          continue;
        }

        if( p0 * total_interests < satisfied_interests)//means the update procedure made the face already reliable
        {
          NS_LOG_DEBUG( "n["<< *it << "] = skipped, \t We think the previous update procedure made the face already reliable");
          continue;
        }

        n = log((satisfied_interests/rel_t) - satisfied_interests);
        n -= log((p0 * total_interests) - satisfied_interests);
        n /= log(1-ema_alpha);

        NS_LOG_DEBUG( "n["<< *it << "] = [ ln(S(i)/t[i] - S(i)) - ln(p(i)*I - S(i))] / ln(1 - ema_alpha(i)) = "
                      << " [ ln(" << satisfied_interests << "/" << rel_t << "-"  << satisfied_interests << ") - "
                      << "ln(" << p0 << "*" << total_interests << " - " << satisfied_interests << ")] / "
                      << "ln(1-" << ema_alpha << ") = " << n);

        n_max = std::max(n_max, n);
      }
      NS_LOG_DEBUG("Layer " << layer << " is under observation for std::min(" << n_max << "," << MAX_OBSERVATION_PERIODS << ") steps") ;
      observed_layers[layer] = ceil(std::min(n_max, MAX_OBSERVATION_PERIODS));
    }
  }

  std::sort(adp_layers.begin (),adp_layers.end ()); // just for the case they are not sorted in ascending order...
  for(std::vector<int>::iterator it = adp_layers.begin (); it != adp_layers.end (); ++it)
  {
    NS_LOG_DEBUG("Observation Phase for layer " << *it << " is over. Dropping traffic will be shifted\n");
    int curLayer = *it;
    int droppingLayer = getDroppingLayer ();

    while(curLayer < droppingLayer)
    {
      // interest forwared to the dropping face of curLayer
      double theta = table(determineRowOfFace (DROP_FACE_ID), curLayer) * smeasure->getTotalForwardedInterests (curLayer);

      //max traffic that can be shifted towards face(s) of last
      double chi = (1.0 - table(determineRowOfFace (DROP_FACE_ID), droppingLayer)) * smeasure->getTotalForwardedInterests (droppingLayer);

      if (theta == 0)
      {
        for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
        {
          if(*it == DROP_FACE_ID)
            table(determineRowOfFace (*it), curLayer) = 0;
          else
            table(determineRowOfFace (*it), curLayer) = 1.0 / (faces.size ()-1);
        }
      }
      else if(chi == 0) // nothing has been forwarded via the last face, set drop prob to 100%
      {
        for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
        {
          if(*it == DROP_FACE_ID)
            table(determineRowOfFace (*it), droppingLayer) = 1;
          else
            table(determineRowOfFace (*it), droppingLayer) = 0;
        }
      }
      else
      {
        if(chi > theta) //if we can shift more than we need to, we just shift how much we need
          chi = theta;

        //reduce dropping prob for lower layer
        table(determineRowOfFace (DROP_FACE_ID), curLayer) -= (chi / smeasure->getTotalForwardedInterests (curLayer));

        //increase dropping prob for higher layer
        table(determineRowOfFace (DROP_FACE_ID), droppingLayer)  += (chi / smeasure->getTotalForwardedInterests (droppingLayer));

        //calc n_frist , n_last normalization value without dropping face;
        double n_first = 0;
        double n_last = 0;

        for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
        {
          if(*it == DROP_FACE_ID)
            continue;

          n_first += table(determineRowOfFace (*it), curLayer);
          n_last +=  table(determineRowOfFace (*it), droppingLayer);
        }

        //increase & decrease other faces accordingly
        for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
        {
          if(*it == DROP_FACE_ID)
            continue;

          table(determineRowOfFace (*it), curLayer) += (chi/smeasure->getTotalForwardedInterests (curLayer)) * (table(determineRowOfFace (*it), curLayer)/n_first);
          table(determineRowOfFace (*it), droppingLayer) -= (chi/smeasure->getTotalForwardedInterests (droppingLayer)) * (table(determineRowOfFace (*it), droppingLayer)/n_last);
        }
      }

      //check if we can break the while loop already
      if(droppingLayer == getDroppingLayer ())
        break;
      else
        droppingLayer = getDroppingLayer ();
    }
  }
}

int SAFForwardingTable::getDroppingLayer()
{
  for(int i = (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS") - 1; i >= 0; i--) // for each layer
  {
    if(table(determineRowOfFace (DROP_FACE_ID), i) < 1.0)
      return i;
  }
  return 0;
}

int SAFForwardingTable::determineRowOfFace(int face_uid)
{
  return determineRowOfFace (face_uid, table, faces);
}

int SAFForwardingTable::determineRowOfFace(int face_id, boost::numeric::ublas::matrix<double> tab, std::vector<int> faces)
{
  // check if table fits to faces
  if(tab.size1 () != faces.size ())
  {
    fprintf(stderr, "Error the number of faces dont correspond to the table!\n");
    return FACE_NOT_FOUND;
  }

  //determine row of face
  int faceRow = FACE_NOT_FOUND;
  std::sort(faces.begin(), faces.end());//order

  int rowCounter = 0;
  for(std::vector<int>::iterator i = faces.begin (); i != faces.end() ; ++i)
  {
    //fprintf(stderr, "*i=%d ; face_id=%d\n",*i,face_id);
    if(*i == face_id)
    {
      faceRow = rowCounter;
      break;
    }
    rowCounter++;
  }
  return faceRow;
}

matrix<double> SAFForwardingTable::removeFaceFromTable (int faceId, matrix<double> tab, std::vector<int> faces)
{
  int faceRow = determineRowOfFace (faceId, tab, faces);

  if(faceRow == FACE_NOT_FOUND)
  {
    fprintf(stderr, "Could not remove Face from Table as it does not exist");
    return tab;
  }

  //fprintf(stderr, "facerow=%d\ņ",faceRow);
  //fprintf(stderr, "tab.size1=%d ; tab.size2=%d\n",tab.size1 (), tab.size2 ());

  matrix<double> m (tab.size1 () - 1, tab.size2 ());
  for (unsigned j = 0; j < tab.size2 (); ++j) /* columns */
  {
    for (unsigned i = 0; i < tab.size1 (); ++i) /* rows */
    {
      if(i < (unsigned int) faceRow)
      {
        m(i,j) = tab(i,j);
      }
      /*else if(faceRow == i)
      {
        // skip i-th row.
      }*/
      else if (i > (unsigned int) faceRow)
      {
        m(i-1,j) = tab(i,j);
      }
    }
  }
  return normalizeColumns (m);
}

matrix<double> SAFForwardingTable::normalizeColumns(matrix<double> m)
{
  for (unsigned j = 0; j < m.size2 (); ++j) /* columns */
  {
    double colSum= 0;
    for (unsigned i = 0; i < m.size1 (); ++i) /* rows */
    {
      if(m(i,j) > 0)
        colSum += m(i,j);
    }
    if(colSum == 0) // means we have removed the only face that was able to transmitt the traffic
    {
      //split probabilities
      for (unsigned i = 0; i < m.size1 (); ++i) /* rows */
        m(i,j) = 1.0 /((double)m.size1 ());
    }
    else
    {
      for (unsigned i = 0; i < m.size1 (); ++i) /* rows */
      {
        if(m(i,j) < 0)
          m(i,j) = 0;
        else
          m(i,j) /= colSum;
      }
    }
  }
  return m;
}

int SAFForwardingTable::chooseFaceAccordingProbability(matrix<double> m, int ilayer, std::vector<int> faceList)
{
  double rvalue = randomVariable.GetValue ();
  double sum = 0.0;

  if(faceList.size () != m.size1 ())
  {
    fprintf(stderr, "Error ForwardingMatrix invalid cant choose Face\n!");
    return DROP_FACE_ID;
  }

  for(unsigned int i = 0; i < m.size1 (); i++)
  {
    sum += m(i, ilayer);
    if(rvalue <= sum)
    {
      return faceList.at (i);
    }
  }
  //error case
  return DROP_FACE_ID;
}

void SAFForwardingTable::increaseReliabilityThreshold(int layer)
{
  updateReliabilityThreshold (layer, true);
}

void SAFForwardingTable::decreaseReliabilityThreshold(int layer)
{
  updateReliabilityThreshold (layer, false);
}

void SAFForwardingTable::updateReliabilityThreshold(int layer, bool increase)
{
  ParameterConfiguration *p = ParameterConfiguration::getInstance ();

  double new_t = 0.0;

  if(increase)
    new_t = curReliability[layer] + ((p->getParameter("RELIABILITY_THRESHOLD_MAX") - curReliability[layer]) * p->getParameter("LAMBDA"));
  else
    new_t = curReliability[layer] - ((curReliability[layer] - p->getParameter("RELIABILITY_THRESHOLD_MIN")) * p->getParameter("LAMBDA"));

  if(new_t > p->getParameter("RELIABILITY_THRESHOLD_MAX"))
    new_t = p->getParameter("RELIABILITY_THRESHOLD_MAX");

  if(new_t < p->getParameter("RELIABILITY_THRESHOLD_MIN"))
    new_t = p->getParameter("RELIABILITY_THRESHOLD_MIN");

  if(new_t != curReliability[layer])
  {
  if(increase)
    NS_LOG_DEBUG("Increasing reliability[" << layer << "]=" << new_t);
  else
    NS_LOG_DEBUG("Decreasing reliability[" << layer << "]=" << new_t);
  }

  curReliability[layer] = new_t;
}

void SAFForwardingTable::addFace(shared_ptr<Face> face)
{
  faces.push_back (face->getId());
  std::sort(faces.begin(), faces.end());//order

  matrix<double> m (table.size1 () + 1, table.size2 ());

  int faceRow = determineRowOfFace (face->getId(), m, faces);

  for (unsigned int j = 0; j < table.size2 (); ++j) /* columns */
  {
    for (unsigned int i = 0; i < table.size1 (); ++i) /* rows */
    {
      if(i < (unsigned int) faceRow)
      {
        m(i,j) = table(i,j);
      }
      else if((unsigned int) faceRow == i)
      {
        //m(i,j) = 1.0 / (double)(faces.size () - 1);
        m(i,j) = 0.0;
      }
      else if (i > (unsigned int) faceRow)
      {
        m(i+1,j) = table(i,j);
      }
    }
    if( (unsigned int) faceRow == table.size1())
      m(faceRow,j) = 0.0;
  }
  table = normalizeColumns (m);
}

void SAFForwardingTable::removeFace(shared_ptr<Face> face)
{
  int faceRow = determineRowOfFace (face->getId());

  if(faceRow == FACE_NOT_FOUND)
  {
    NS_LOG_DEBUG("Could not remove Face from Table as it does not exist");
    return;
  }

  matrix<double> m (table.size1 () - 1, table.size2 ());

  for (unsigned int j = 0; j < table.size2 (); ++j) /* columns */
  {
    for (unsigned int i = 0; i < table.size1 (); ++i) /* rows */

      if(i < (unsigned int) faceRow)
      {
        m(i,j) = table(i,j);
      }
      /*else if(faceRow == i)
      {
        // skip i-th row.
      }*/
      else if (i > (unsigned int) faceRow)
      {
        m(i-1,j) = table(i,j);
      }
    }

  faces.erase(std::find(faces.begin (),faces.end (),face->getId()));
  table = normalizeColumns (m);
}
