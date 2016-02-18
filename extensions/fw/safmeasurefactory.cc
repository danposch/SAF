#include "safmeasurefactory.h"

using namespace nfd;
using namespace nfd::fw;

SAFMeasureFactory* SAFMeasureFactory::instance = NULL;

SAFMeasureFactory::SAFMeasureFactory()
{
}

SAFMeasureFactory* SAFMeasureFactory::getInstance()
{
  if(instance == NULL)
    instance = new SAFMeasureFactory();

  return instance;
}

boost::shared_ptr<SAFStatisticMeasure> SAFMeasureFactory::getMeasure(std::string name, std::vector<int> faces)
{
  MeasureMap::iterator match = mmap.end ();
  unsigned int matching_chars = 0;
  unsigned int longest_match = 0;

  for(MeasureMap::iterator it = mmap.begin (); mmap.end () != it; it++)
  {
    if(it->first.size() > name.size ()) //cant match
      continue;

    matching_chars = 0;
    while(matching_chars < it->first.size() && it->first[matching_chars] == name[matching_chars])
      matching_chars++;

    if(matching_chars > 0 // is match
       && (matching_chars == name.size () || ((matching_chars < name.size () && name[matching_chars] == '/' )|| it->first.size() == 1)) //is full component match
       && longest_match < matching_chars) // is longer match
    {
      longest_match = matching_chars;
      match = it;
    }
  }

  if(mmap.end () != match)
  {
    //fprintf(stderr, "Found match for %s: %s\n", name.c_str (), match->first.c_str());

    switch (match->second)
    {
      case SAFStatisticMeasure::MThroughput:
      {
        return boost::shared_ptr<Mratio>(new Mratio(faces));
      }
      case SAFStatisticMeasure::MDelay:
      {
        //check for known attributes
        int default_delay = 1000; //ms

        MeasureAttributeMap::iterator it = amap.find (match->first);

        if(it != amap.end ())
        {
          for(std::map<std::string, std::string>::iterator k = it->second.begin(); k != it->second.end(); k++)
          {
            if(k->first.compare("MaxDelayMS") == 0)
            {
              default_delay = boost::lexical_cast<int>(k->second);
            }
          }
        }
        return boost::shared_ptr<Mratio>(new MDelay(faces, default_delay));
      }
    case SAFStatisticMeasure::MHop:
    {
      //check for known attributes
      int max_hops = 10; //ms

      MeasureAttributeMap::iterator it = amap.find (match->first);

      if(it != amap.end ())
      {
        for(std::map<std::string, std::string>::iterator k = it->second.begin(); k != it->second.end(); k++)
        {
          if(k->first.compare("MaxHops") == 0)
          {
            max_hops = boost::lexical_cast<int>(k->second);
          }
        }
      }
      return boost::shared_ptr<Mratio>(new MHop(faces, max_hops));
    }
      default:
        return boost::shared_ptr<Mratio>(new Mratio(faces));
    }
  }
  else
    return boost::shared_ptr<Mratio>(new Mratio(faces));
}

void SAFMeasureFactory::registerAttribute(std::string prefix, std::string attribute, std::string value)
{
  amap[prefix][attribute] = value;
}

void SAFMeasureFactory::registerMeasure(std::string prefix, SAFStatisticMeasure::MeasureType type)
{
  mmap[prefix] = type;
}
