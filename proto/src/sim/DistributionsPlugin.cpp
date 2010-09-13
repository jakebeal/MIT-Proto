/* 
 * File:   DistributionsPlugin.cpp
 * Author: prakash
 * 
 * Created on February 5, 2010, 6:47 PM
 */

#include <string>
#include <sstream>
#include <iostream>
using namespace std;
#include "DistributionsPlugin.h"
#include "XGrid.h"
#include "FixedPoint.h"
#include "Grid.h"
#include "GridRandom.h"
#include "Cylinder.h"
#include "Torus.h"

#define DLL_NAME "libdistributions"
#define D_GRID "grid"
#define D_GRIDEPS "grideps"
#define D_XGRID "xgrid"
#define D_FIXEDPT "fixedpt"
#define D_CYLINDER "cylinder"
#define D_TORUS "torus"

/*************** Library ***************/
void* DistributionsPlugin::get_sim_plugin(string type, string name, Args* args,
                                          SpatialComputer* cpu, int n) {
  if(type==DISTRIBUTION_PLUGIN) {
    if(name==D_GRID) // plain grid    
      return new Grid(n,cpu->volume);
    if(name==D_GRIDEPS) // randomized grid
      return new GridRandom(args,n,cpu->volume);
    if(name==D_XGRID) // grid w. random y      
      return new XGrid(n,cpu->volume);
    if(name==D_FIXEDPT) // specify location of first k devices
      return new FixedPoint(args,n,cpu->volume);
    if(name==D_CYLINDER)
      return new Cylinder(n,cpu->volume);
    if(name==D_TORUS)
      return new Torus(args,n,cpu->volume);
  } 
  return NULL;
}

string DistributionsPlugin::inventory() {
  string s = "";
  s += registry_entry(DISTRIBUTION_PLUGIN,D_GRID,DLL_NAME);
  s += registry_entry(DISTRIBUTION_PLUGIN,D_GRIDEPS,DLL_NAME);
  s += registry_entry(DISTRIBUTION_PLUGIN,D_XGRID,DLL_NAME);
  s += registry_entry(DISTRIBUTION_PLUGIN,D_FIXEDPT,DLL_NAME);
  s += registry_entry(DISTRIBUTION_PLUGIN,D_CYLINDER,DLL_NAME);
  s += registry_entry(DISTRIBUTION_PLUGIN,D_TORUS,DLL_NAME);
  return s;
}

extern "C" {
  ProtoPluginLibrary* get_proto_plugin_library()
  { return new DistributionsPlugin(); }
  const char* get_proto_plugin_inventory()
  { return (new string(DistributionsPlugin::inventory()))->c_str(); }
}



