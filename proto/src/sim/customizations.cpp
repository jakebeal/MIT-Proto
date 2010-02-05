/* User plug-in point for simulator customizations
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// This file is where to make all of the small changes to adapt
// a spatial computer to the needs of a particular user.
// It's a separate file so that the core files won't change often.
// In particular, it is the only place that should need to be
// modified in order to integrate a new layer with the body.

#include <dlfcn.h>
#include <string>

#include "config.h"
#include "spatialcomputer.h"
#include "customizations.h"
#include "basic-hardware.h"
#include "simpledynamics.h"
#include "unitdiscradio.h"
#include "wormhole-radio.h"
#include "multiradio.h"
#include "Distribution.h"
#include "XGrid.h"
/*****************************************************************************
 *  DEVICE DISTRIBUTIONS                                                     *
 *****************************************************************************/
class UniformRandom : public Distribution {
public:
  UniformRandom(int n, Rect* volume) : Distribution(n,volume) {}
  virtual BOOL next_location(METERS *loc) {
    loc[0] = urnd(volume->l,volume->r);
    loc[1] = urnd(volume->b,volume->t);
    if(volume->dimensions()==3) {
      Rect3* r = (Rect3*)volume;
      loc[2] = urnd(r->f,r->c);
    } else loc[2]=0;
    return TRUE;
  }
};



class FixedPoint : public UniformRandom {
public:
  int fixed; int n_fixes;
  Population fixes;
  FixedPoint(Args* args, int n, Rect* volume) : UniformRandom(n,volume) {
    fixed=0; n_fixes=0;
    do {
      n_fixes++;
      METERS* loc = (METERS*)calloc(3,sizeof(METERS));
      loc[0]=args->pop_number(); loc[1]=args->pop_number();
      loc[2]= (str_is_number(args->peek_next()) ? args->pop_number() : 0);
      fixes.add(loc);
    } while(args->extract_switch("-fixedpt",FALSE));
  }
  virtual ~FixedPoint() {
    for(int i=0;i<n_fixes;i++) { free(fixes.get(i)); }
  }
  BOOL next_location(METERS *loc) {
    if(fixed<n_fixes) {
      METERS* src = (METERS*)fixes.get(fixed);
      for(int i=0;i<3;i++) loc[i]=src[i];
      fixed++;
      return TRUE;
    } else {
      return UniformRandom::next_location(loc);
    }
  }
};

class Grid : public Distribution {
public:
  int rows,columns,layers;
  int i;
  Grid(int n, Rect* volume) : Distribution(n,volume) {
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
  BOOL next_location(METERS *loc) {
    int l = (i%layers), r = (i/layers)%rows, c = (i/(layers*rows));
    loc[0] = volume->l + c*width/columns;
    loc[1] = volume->b + r*height/rows;
    loc[2] = (volume->dimensions()==3)?(((Rect3*)volume)->f+l*depth/layers):0;
    i++;
    return TRUE;
  }
};

class GridRandom : public Grid {
public:
  METERS epsilon;
  GridRandom(Args* args, int n, Rect* volume) : Grid(n,volume) {
    epsilon = args->pop_number();
  }
  BOOL next_location(METERS *loc) {
    Grid::next_location(loc);
    loc[0] += epsilon*((rand()%1000/1000.0) - 0.5);
    loc[1] += epsilon*((rand()%1000/1000.0) - 0.5);
    if(volume->dimensions()==3) loc[2] += epsilon*((rand()%1000/1000.0) - 0.5);
    i++;
    return TRUE;
  }
};

class Cylinder : public Distribution {
public:
  METERS r;
  Cylinder(int n, Rect* volume) : Distribution(n,volume) {
    r = height / 2;
  }
  BOOL next_location(METERS *loc) {
    loc[0] = urnd(volume->l,volume->r);
    flo theta = urnd(0, 2 * 3.14159);
    loc[1] = r * sin(theta);
    loc[2] = r * cos(theta);
    return TRUE;
  }
};

const flo TWO_PI = 2 * M_PI;

class Torus : public Distribution {
public:
  METERS r, r_inner;
  Torus(Args* args, int n, Rect *volume) : Distribution(n, volume) {
    METERS outer = MIN(width, height) / 2;
    flo ratio = (str_is_number(args->peek_next()) ? args->pop_number() : 0.75);
    r = ratio * outer;
    r_inner = outer - r;
  }
  
  BOOL next_location(METERS *loc) {
    flo theta = urnd(0, TWO_PI);
    if(volume->dimensions() == 3) {
      flo phi = urnd(0, TWO_PI);
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
};

/*****************************************************************************
 *  CHOICE OF DISTRIBUTION                                                   *
 *****************************************************************************/
void SpatialComputer::choose_distribution(Args* args, int n) {
  // Note: dist_volume isn't garbage collected, but it's just 1 rect per SC
  Rect *dist_volume = volume->clone();
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
  
  if(args->extract_switch("-grideps")) { // random grid
    distribution = new GridRandom(args,n,dist_volume);
    args->extract_switch("-grid"); // with eps, -grid is a null operation
  } else if(args->extract_switch("-grid")) { // plain grid
    distribution = new Grid(n,dist_volume);
  } else if(args->extract_switch("-xgrid")) { // grid w. random y
    distribution = new XGrid(n,dist_volume);
  } else if(args->extract_switch("-fixedpt")) {
    distribution = new FixedPoint(args,n,dist_volume);
  } else if(args->extract_switch("-cylinder")) {
    distribution = new Cylinder(n,dist_volume);
  } else if(args->extract_switch("-torus")) {
    distribution = new Torus(args,n,dist_volume);
  } else { // default is random
    distribution = new UniformRandom(n,dist_volume);
  }
}


/*****************************************************************************
 *  TIME MODELS                                                              *
 *****************************************************************************/
class FixedTimer : public DeviceTimer {
  SECONDS dt, half_dt, internal_dt, internal_half_dt;
  flo ratio;
public:
  FixedTimer(flo dt, flo ratio) { 
    this->dt=dt; half_dt=dt/2; 
    internal_dt = dt*ratio; internal_half_dt = dt*ratio/2;
    this->ratio = ratio;
  }
  void next_transmit(SECONDS* d_true, SECONDS* d_internal) {
    *d_true = half_dt; *d_internal = internal_half_dt;
  }
  void next_compute(SECONDS* d_true, SECONDS* d_internal) {
    *d_true = dt; *d_internal = internal_dt;
  }
  DeviceTimer* clone_device() { return new FixedTimer(dt,internal_dt/dt); }
  void set_internal_dt(SECONDS dt) {
    internal_dt = dt; 
    internal_half_dt = dt/2; 
    this->dt = internal_dt/ratio; 
    half_dt = internal_dt/(ratio*2); 
  } 
};

class FixedIntervalTime : public TimeModel, public HardwarePatch {
  BOOL sync;
  flo dt; flo var;
  flo ratio; flo rvar;  // ratio is internal/true time
public:
  FixedIntervalTime(Args* args, SpatialComputer* p) {
    sync = args->extract_switch("-sync");
    dt = (args->extract_switch("-desired-period"))?args->pop_number():1;
    var = (args->extract_switch("-desired-period-variance"))
      ? args->pop_number() : 0;
    ratio = (args->extract_switch("-desired-ratio"))?args->pop_number():1;
    rvar = (args->extract_switch("-desired-ratio-variance"))
      ? args->pop_number() : 0;
    
    p->hardware.patch(this,SET_DT_FN);
  }
  
  DeviceTimer* next_timer(SECONDS* start_lag) {
    if(sync) { *start_lag=0; return new FixedTimer(dt,ratio); }
    *start_lag = urnd(0,dt);
    flo p = urnd(dt-var,dt+var);
    flo ip = urnd(ratio-rvar,ratio+rvar);
    return new FixedTimer(MAX(0,p),MAX(0,ip));
  }
  SECONDS cycle_time() { return dt; }
  NUM_VAL set_dt (NUM_VAL dt) { 
    ((FixedTimer*)device->timer)->set_internal_dt(dt); 
    return dt;
  }
};


/*****************************************************************************
 *  CHOICE OF TIME MODELS                                                    *
 *****************************************************************************/
void SpatialComputer::choose_time_model(Args* args, int n) {
  time_model = new FixedIntervalTime(args,this);
}


const char *dl_exts[] = { ".so", ".dylib", NULL};

void *dlopenext(const char *name, int flag) {
    const char **ext = dl_exts;
    void *hand = NULL;

    while(*ext) {
        std::string search = name;
        hand = dlopen((search + (*ext)).c_str(), flag);
        if (hand) break;
        ext++;
    }

    return hand;
}




// have moved function dlopenext to spatialcomputer.cpp
void SpatialComputer::add_plugin(const char *name, Args *args, int n) {
  void *hand;
  layer_getter getter;
  Layer *layer;
  int mask;

  hand = dlopenext(name, RTLD_NOW);
  if(!hand) {
    std::string search = name;
    if(search.find('/') == std::string::npos) {
      search = "proto-" + search;
      hand = dlopenext(search.c_str(), RTLD_NOW);
    }
  }
  if(!hand)
    uerror("Unable to load plugin: %s", dlerror());

  getter = (layer_getter)dlsym(hand, "get_layer");

  if(getter == NULL) {
    uerror("Unable to load get_layer from plugin %s: %s", name, dlerror());
  }

  layer = getter(args, this, n);
  mask = layer->get_type();
  if(layer_mask & mask) {
    uerror("Layer %s conflicts with previously loaded layer!", name);
  }

  if(mask & LAYER_PHYSICS) {
    physics = (BodyDynamics*) layer;
  }
  if(mask & LAYER_TIME) {
    time_model = (TimeModel*) layer;
  }

  if(!(mask & ~LAYER_RADIO)) {
    addLayer(layer);
  }

  layer_mask |= mask;
}


/*****************************************************************************
 *  CHOICE OF LAYERS                                                         *
 *****************************************************************************/
// boot_layers is the one function that needs to know about all possible layers
// A layer that is not added at this time cannot be added layer; it may be
// added but not executed, though.
void SpatialComputer::choose_layers(Args* args, int n) {
  layer_mask = 0;

  if(args->extract_switch("-ode")) {
    add_plugin("ode", args, n);
  }

  addLayer(new DebugLayer(args,this));
  addLayer(new SimpleLifeCycle(args,this));
  addLayer(new PerfectLocalizer(this));

  // other layers
  if(args->extract_switch("-mote-io")) addLayer(new MoteIO(args,this));

  if(args->extract_switch("-plugins")) {
    char *plugins = args->pop_next();
    char *name = strtok(plugins, ",");

    while(name != NULL) {
      add_plugin(name, args, n);
      name = strtok(NULL, ",");
    }
  }

  if(!(layer_mask & LAYER_RADIO)) {
    if(args->extract_switch("-multiradio")) {
      MultiRadio * r = new MultiRadio(args, this, n);
      addLayer(r);

      if(args->extract_switch("-wormholes")) {
        WormHoleRadio *w = new WormHoleRadio(args,this,n);
        addLayer(w); r->add_radio(w);
      }

      UnitDiscRadio *d = new UnitDiscRadio(args, this, n);
      addLayer(d); r->add_radio(d);

    } else if(args->extract_switch("-wormholes")) {
      addLayer(new WormHoleRadio(args,this,n));
    } else {
      addLayer(new UnitDiscRadio(args,this,n));
    }
  }


  if(!(layer_mask & LAYER_PHYSICS)) {
    physics = new SimpleDynamics(args,this,n);
  }



  choose_distribution(args, n);

  if(!(layer_mask & LAYER_TIME)) {
    choose_time_model(args, n);
  }
}
