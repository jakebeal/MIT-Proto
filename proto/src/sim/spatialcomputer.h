/* Top-level spatial computer classes and plug-in templates
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __SPATIALCOMPUTER__
#define __SPATIALCOMPUTER__

#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <exception>
#include <dlfcn.h>
#include "sim-hardware.h"
#include "utils.h"
#include "proto_platform.h"
#include "scheduler.h"
#include "Distribution.h"
#include "LibRegistry.h"
#include "DeviceTimer.h"
#include "TimeModel.h"
#include "FixedIntervalTime.h"

// prototype classes
class Device; class SpatialComputer;
class DllNotFoundException;


/*****************************************************************************
 *  TIME AND SPACE DISTRIBUTIONS                                             *
 *****************************************************************************/

/*****************************************************************************
 *  DYNAMICS                                                                 *
 *****************************************************************************/
// this is the global aspect of a dynamics layer
enum layer_type {
  LAYER_OTHER        = 0x0,
  LAYER_RADIO        = 0x1,
  LAYER_PHYSICS      = 0x2,
  LAYER_TIME         = 0x4,
  /* Distributions are not currently Layer s, so we can' load them
     using the current dynamic loading system. */
  /*  LAYER_DISTRIBUTION = 0x8, */
};

class Layer : public EventConsumer {
 public:
  int id;          // what number layer this is, for lookup during callbacks
  BOOL can_dump;   // -ND[layer] is expected to turn off dumping for a layer
  SpatialComputer* parent;
  Layer(SpatialComputer* p);
  virtual ~Layer() {} // make sure that destruction is passed to subclasses
  virtual BOOL handle_key(KeyEvent* key) {return FALSE;}
  virtual void visualize() {}
  virtual BOOL evolve(SECONDS dt) { return FALSE; }
  virtual void add_device(Device* d)=0;    // may add a DeviceLayer to Device
  virtual void device_moved(Device* d) {}  // adjust for device motion
  // removal, updates handled through DeviceLayer
  virtual void dump_header(FILE* out) {} // field names in ""s for a data file
  virtual layer_type get_type() { return LAYER_OTHER; }
};

// this is the device-specific instantiation of a layer
class DeviceLayer : public EventConsumer {
 public:
  Device* container;
  DeviceLayer(Device* container) { this->container=container; }
  virtual ~DeviceLayer() {}; // make sure destruction cascades correctly
  virtual void preupdate() {}  // to called before computation
  virtual void update() {}  // to called after a computation
  virtual void visualize() {} // to be called at visualization
  virtual BOOL handle_key(KeyEvent* event) { return FALSE; }
  virtual void copy_state(DeviceLayer* src)=0; // to be called during cloning
  virtual void dump_state(FILE* out, int verbosity) {}; // print state to file
};

// The Body/BodyDynamics is a layer that is stored and managed
// specially because it tracks the position of the device in space.
class Body : public DeviceLayer {
 public:
  BOOL moved;
  Body(Device* container) : DeviceLayer(container) {}
  virtual ~Body() {}; // make sure destruction cascades correctly
  // on delete, a body should remove itself from the BodyDynamics
  virtual const flo* position()=0; // returns a 3-space coordinate
  virtual const flo* velocity()=0; // returns a 3-space vector
  virtual const flo* orientation()=0; // returns a quaternion
  virtual const flo* ang_velocity()=0; // returns a 3-space vector
  virtual void set_position(flo x, flo y, flo z)=0;
  virtual void set_velocity(flo dx, flo dy, flo dz)=0;
  virtual void set_orientation(const flo *q)=0;
  virtual void set_ang_velocity(flo dx, flo dy, flo dz)=0;
  virtual flo display_radius()=0; // bigger bodies get bigger displays
  virtual void render_selection()=0;  // render for selection
  void copy_state(DeviceLayer* src) {} // required virtual is moot
};

// BodyDynamics is a special type of layer, implementing the base physics
class BodyDynamics : public Layer {
 public:
  BodyDynamics(SpatialComputer* p) : Layer(p) {}
  virtual ~BodyDynamics() {} // make sure destruction is passed to subclasses
  virtual Body* new_body(Device* d, flo x, flo y, flo z)=0;
  void add_device(Device* d) {} // required virtual, replaced by new_body
};

/*****************************************************************************
 *  SPATIAL COMPUTER                                                         *
 *****************************************************************************/
enum DeviceEvent { COMPUTE, BROADCAST };

class Device : public EventConsumer {
  static int top_uid;               // uids are generated in rising sequence
 public:
  int uid, backptr;                 // internal (& ext.) identifier for device
  SECONDS run_time;                 // how much internal time has elapsed?
  DeviceTimer* timer;               // supplies delays for compute, broadcast
  Body* body;                       // the physical part of the device
  int num_layers;                   // integration with dynamics
  DeviceLayer** layers;             // integration with dynamics
  MACHINE* vm;                      // the Proto kernel
  SpatialComputer* parent;          // upward track for the device
  BOOL is_selected;                 // is this device currently selected?
  BOOL is_debug;                    // is this device currently a debug focus?
  
  Device(SpatialComputer* parent, METERS *loc, DeviceTimer *timer);
  ~Device();
  Device* clone_device(METERS *loc); // make a clone at location loc
  void internal_event(SECONDS time, DeviceEvent type); // broadcast or compute
  void text_scale();                // scale to display text about device
  void load_script(uint8_t* script, int len);
  BOOL handle_key(KeyEvent* key);
  virtual void visualize();
  virtual void render_selection(); // render for selection
  virtual void dump_state(FILE* out, int verbosity);
  BOOL debug();
};

// a request for cloning carries info about location and source, too
struct CloneReq {
  int id; Device* parent;
  METERS child_pos[3];
  CloneReq(int id, Device* parent, const flo* cp) {
    this->id=id; this->parent=parent;
    for(int i=0;i<3;i++) child_pos[i] = cp[i];
  }
};

class SpatialComputer : public EventConsumer {
 public:
  // display variables
  BOOL is_show_val, is_show_vec, is_show_id, is_show_version;
  BOOL is_debug, is_dump_default, is_dump_hood, is_dump_value; 
  flo display_mag; // magnifier for body display
  Population selection;     // the list of devices currently selected
  // dumping variables
  BOOL is_dump, is_probe_filter, is_show_snaps, just_dumped;
  SECONDS dump_start, dump_period, next_dump, snap_vis_time;
  const char* dump_dir;  // directory where dumps will go
  const char* dump_stem; // start of the dump file name
  
  // system state
  SECONDS sim_time;         // time (initially zero)
  TimeModel *time_model;    // how time evolves on each device
  Distribution *distribution; // how devices are scattered in space
  Rect* volume;             // space (start bounds: fixed, but may be exceeded)
  Population devices;       // computation (set of devices)
  BodyDynamics *physics;    // dynamics of bodies, always first evaluated
  Population dynamics;      // additional types of physics
  Scheduler* scheduler;     // "priority queue" for device events
  SimulatedHardware hardware; // patch connecting VMs and dynamics
  int version;              // what software version is currently running

  std::queue<int> death_q;  // nodes requesting to suicide
  std::queue<CloneReq*> clone_q;  // nodes requesting to reproduce
  static const string registryFilePath;
  static const char* dl_exts[];

 public:
  SpatialComputer(Args* args);
  ~SpatialComputer();
  void load_script(uint8_t* script, int len);
  void load_script_at_selection(uint8_t* script, int len);
  // EventConsumer routines
  BOOL handle_key(KeyEvent* key);
  BOOL handle_mouse(MouseEvent* mouse);
  void visualize();
  BOOL evolve(SECONDS limit);
  // selection routines
  void update_selection();
  void render_selection(); // render for selection
  void drag_selection(flo* delta);
  void dump_header(FILE* out); // print a header for a Matlab-style data file
  void dump_state(FILE* out); // print log info for all devices
  void dump_selection(FILE* out, int verbosity);
  void dump_frame(SECONDS time, BOOL time_in_name);
  BOOL is_3d() { return volume->dimensions()>2; }
  virtual Layer* find_layer(char* name, Args* args, int n);
  virtual TimeModel* find_time_model(char* name, Args* args, int n);
  virtual Distribution* find_distribution(char* name, Args* args, int n);
  virtual void  initializePlugins(Args* args, int n);
  void setDefaultTimeModel(Args* args, int n);
  void setDefaultDistribution(Args* args, int n);
  
 private:
  void get_volume(Args* args, int n); // shared dist constructor
  void choose_distribution(Args* args, int n); // custom dist selector
  void choose_layers(Args* args, int n); // custom layer selector
  void choose_time_model(Args* args, int n); // custom timing selector
  int addLayer(Layer* layer); // add a layer to dynamics & set callback vars

  void add_plugin(const char *name, Args *args, int n);
  void readRegistry (fstream& fin, LibRegistry& out);
  void* getDLLHandle(string dllName);
  void* dlopenext(const char *name, int flag);
  int layer_mask;
  LibRegistry mLibReg;
  map<string,void*> mLoadedDLLMap;
  };

class DllNotFoundException: public exception
{
public:
	string message;
	DllNotFoundException(string msg);
	~DllNotFoundException() throw();
	virtual const char* what() const throw();
};

// global variable set to the spatial computer during visualize(),
// to avoid passing it around continually
extern SpatialComputer* vis_context;

typedef Layer* (*layer_getter) (Args *args, SpatialComputer *cpu, int n);



#endif // __SPATIALCOMPUTER__
