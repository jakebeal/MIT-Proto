/* 
 * File:   SimpleLifeCycle.h
 * Author: prakash
 *
 * Created on February 8, 2010, 12:27 PM
 */

#ifndef _SIMPLELIFECYCLE_H
#define	_SIMPLELIFECYCLE_H

#include "spatialcomputer.h"

/*****************************************************************************
 *  SIMPLE LIFECYCLE                                                         *
 *****************************************************************************/
// clone, death
class SimpleLifeCycle : public Layer, public HardwarePatch {
 public:
  flo clone_delay;               // minimum time between clonings

  SimpleLifeCycle(Args* args, SpatialComputer* parent);
  void add_device(Device* d);
  // hardware patch functions
  void dump_header(FILE* out); // list log-file fields
 private:
  void die_op(MACHINE* machine);
  void clone_op(MACHINE* machine);
  void die (NUM_VAL val);
  void clone_machine (NUM_VAL val);
};

class SimpleLifeCycleDevice : public DeviceLayer {
 public:
  SimpleLifeCycle* parent;
  BOOL clone_cmd;            // request for cloning is active
  flo clone_timer;           // timer for delay between clonings
  SimpleLifeCycleDevice(SimpleLifeCycle* parent, Device* container);
  void update();
  BOOL handle_key(KeyEvent* event);
  void clone_me();
  void copy_state(DeviceLayer* src) {} // to be called during cloning
  void dump_state(FILE* out, int verbosity); // print state to file
};



#endif	/* _SIMPLELIFECYCLE_H */

