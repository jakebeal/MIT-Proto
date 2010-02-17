/* 
 * File:   DistributionsPlugin.cpp
 * Author: prakash
 * 
 * Created on February 5, 2010, 6:47 PM
 */

#include <string>
#include <sstream>
using namespace std;
#include "DistributionsPlugin.h"
#include "XGrid.h"
#include "FixedPoint.h"
#include "Grid.h"
#include "GridRandom.h"
#include "Cylinder.h"
#include "Torus.h"
#include "spatialcomputer.h"



DistributionsPlugin::DistributionsPlugin()
{
    knownDistributions.push_back(string("grideps"));
    knownDistributions.push_back(string("grid"));
    knownDistributions.push_back(string("xgrid"));
    knownDistributions.push_back(string("fixedpt"));
    knownDistributions.push_back(string("cylinder"));
    knownDistributions.push_back(string("torus"));
}
Distribution* DistributionsPlugin::get_distribution(char* name, 
                                                    Args* args,
                                                    SpatialComputer* cpu,
                                                    int n) {
    // Note: dist_volume isn't garbage collected, but it's just 1 rect per SC
  Rect *dist_volume = cpu->volume->clone();
  if(args->extract_switch("-dist-dim")) {
    dist_volume->l = args->pop_number();
    dist_volume->r = args->pop_number();
    dist_volume->b = args->pop_number();
    dist_volume->t = args->pop_number();
    if(dist_volume->dimensions()==3) {
      ((Rect3*)dist_volume)->f = args->pop_number();
      ((Rect3*)dist_volume)->c = args->pop_number();
    }
  }

  if(string(name) == string("grideps")) { // random grid
    return new GridRandom(args,n,dist_volume);
    //args->extract_switch("-grid"); // with eps, -grid is a null operation
  }
  if(string(name) == string("grid")) { // plain grid
    return  new Grid(n,dist_volume);
  }
  if(string(name) == string("xgrid")) { // grid w. random y
    return  new XGrid(n,dist_volume);
  }
  if(string(name) == string("fixedpt")) {
    return  new FixedPoint(args,n,dist_volume);
  }
  if(string(name) == string("cylinder")) {
    return  new Cylinder(n,dist_volume);
  }
  if(string(name) == string("torus")) {
    return  new Torus(args,n,dist_volume);
  } 
  
  // I don't have a distribution that you asked for. Caller must create default(random) distribution.
      return NULL;
}

string DistributionsPlugin::getProperties()
{
    DistributionsPlugin d;
    stringstream ss;
    for(int i = 0; i < d.knownDistributions.size(); i++)
    {
        ss << "Distribution " << d.knownDistributions[i] << " = " << DISTRIBUTIONS_DLL_NAME << endl;
    }
    
    return ss.str();
}

#ifdef __cplusplus

extern "C" {

ProtoPluginLibrary* get_proto_plugins()
{
    return new DistributionsPlugin();
}
const char* get_proto_plugin_properties()
{
    string propS = DistributionsPlugin::getProperties();
    char *props = new char[propS.size() + 1];
    props[propS.size()] = '\0';
    return props;
}

}
#endif


