/* A collection of simple distributions
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <string>
#include <sstream>
#include <iostream>
using namespace std;
#include "DistributionsPlugin.h"

#define DLL_NAME "libdistributions"
#define D_GRID "grid"
#define D_GRIDEPS "grideps"
#define D_XGRID "xgrid"
#define D_FIXEDPT "fixedpt"
#define D_CYLINDER "cylinder"
#define D_TORUS "torus"

/*************** Distribution Code ***************/

// FixedPoint: specify first N locations manually
FixedPoint::FixedPoint(Args* args, int n, Rect* volume):UniformRandom(n,volume){
  fixed=0; n_fixes=0;
  do {
    n_fixes++;
    METERS* loc = (METERS*)calloc(3,sizeof(METERS));
    loc[0]=args->pop_number(); loc[1]=args->pop_number();
    loc[2]= (str_is_number(args->peek_next()) ? args->pop_number() : 0);
    fixes.add(loc);
  } while(args->extract_switch("-fixedpt",FALSE));
}
FixedPoint::~FixedPoint() {
  for(int i=0;i<n_fixes;i++) { free(fixes.get(i)); }
}
BOOL FixedPoint::next_location(METERS *loc) {
  if(fixed<n_fixes) {
    METERS* src = (METERS*)fixes.get(fixed);
    for(int i=0;i<3;i++) loc[i]=src[i];
    fixed++;
    return TRUE;
  } else {
    return UniformRandom::next_location(loc);
  }
}


Grid::Grid(int n, Rect* volume) : Distribution(n,volume) {
  i=0;
  if(volume->dimensions()==3) {
    layers = (int)ceil(pow(n*depth*depth/(width*height), 1.0/3.0));
    rows = (int)ceil(layers*width/depth);
    columns = (int)ceil(n/rows/layers);
  } else {
    rows = (int)ceil(sqrt(n)*sqrt(width/height));
    columns = (int)ceil(n/rows);
    layers = 1;
  }
}
BOOL Grid::next_location(METERS *loc) {
  int l = (i%layers), r = (i/layers)%rows, c = (i/(layers*rows));
  loc[0] = volume->l + c*width/columns;
  loc[1] = volume->b + r*height/rows;
  loc[2] = (volume->dimensions()==3)?(((Rect3*)volume)->f+l*depth/layers):0;
  i++;
  return TRUE;
}


// XGrid: Y is random
XGrid::XGrid(int n, Rect* volume) : Distribution(n,volume) {
  i=0;
  if(volume->dimensions()==3) {
    layers = (int)ceil(pow(n*depth*depth/(width*height), 1.0/3.0));
    rows = (int)ceil(layers*width/depth);
    columns = (int)ceil(n/rows/layers);
  } else {
    rows = (int)ceil(sqrt(n)*sqrt(width/height));
    columns = (int)ceil(n/rows);
    layers = 1;
  }
}
BOOL XGrid::next_location(METERS *loc) {
  int l = (i%layers), r = (i/layers)%rows, c = (i/(layers*rows));
  loc[0] = volume->l + c*width/columns;
  loc[1] = urnd(volume->b,volume->t);
  loc[2] = (volume->dimensions()==3)?(((Rect3*)volume)->f+l*depth/layers):0;
  i++;
  return TRUE;
}


// GridRandom: perturbed grid
GridRandom::GridRandom(Args* args, int n, Rect* volume) : Grid(n,volume) {
  epsilon = args->pop_number();
}
BOOL GridRandom::next_location(METERS *loc) {
  Grid::next_location(loc);
  loc[0] += epsilon*((rand()%1000/1000.0) - 0.5);
  loc[1] += epsilon*((rand()%1000/1000.0) - 0.5);
  if(volume->dimensions()==3) loc[2] += epsilon*((rand()%1000/1000.0) - 0.5);
  return TRUE;
}


Cylinder::Cylinder(int n, Rect* volume) : Distribution(n,volume) {r = height/2;}
BOOL Cylinder::next_location(METERS *loc) {
  loc[0] = urnd(volume->l,volume->r);
  flo theta = urnd(0, 2 * 3.14159);
  loc[1] = r * sin(theta);
  loc[2] = r * cos(theta);
  return TRUE;
}


Torus::Torus(Args* args, int n, Rect *volume) : Distribution(n, volume) {
  METERS outer = MIN(width, height) / 2;
  flo ratio = (str_is_number(args->peek_next()) ? args->pop_number() : 0.75);
  r = ratio * outer;
  r_inner = outer - r;
}
BOOL Torus::next_location(METERS *loc) {
  flo theta = urnd(0, 2*M_PI);
  if(volume->dimensions() == 3) {
    flo phi = urnd(0, 2*M_PI);
    flo rad = urnd(0, r_inner);
    loc[0] = (r + rad * cos(phi)) * cos(theta);
    loc[1] = (r + rad * cos(phi)) * sin(theta);
    loc[2] = rad * sin(phi);
  } else {
    flo rad = r + urnd(-r_inner, r_inner);
    loc[0] = rad * cos(theta);
    loc[1] = rad * sin(theta);
  }
}

/*************** Plugin Library ***************/
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
