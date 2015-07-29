#include "pic.h"

using namespace nfd;
using namespace nfd::fw;

PIC::PIC()
{
  pic = 0;
  avg_pic = 0.0;
  update();
}

PIC::~PIC ()
{

}

void PIC::increase ()
{
  pic++;
}

void PIC::decrease ()
{
  if(pic > 0)
    pic--;
}

void PIC::update ()
{
  avg_pic = ALPHA_PIC * avg_pic + (1.0-ALPHA_PIC) * pic;
  weight = 1.0/(avg_pic+1); //+1 to avoid div by 0
}

double  PIC::getWeight()
{
  return weight;
}
