/* A collection of simple common hardware packages for the simulator
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __BASIC_HARDWARE__
#define __BASIC_HARDWARE__

#include "spatialcomputer.h"
#include <vector>
using namespace std;

/*****************************************************************************
 *  DEBUG                                                                    *
 *****************************************************************************/
// probes, LEDs
class DebugLayer : public Layer, public HardwarePatch {
 public:
  // display parameters
  int n_probes;               // how many debug probes to show?
  BOOL is_show_leds;           // should LEDs be drawn?
  BOOL is_led_rgb;            // true: 1 LED of mixed color; false: 3 LEDs
  BOOL is_led_ghost_mode;     // if rgb=FALSE, LED value sets alpha channel
  BOOL is_led_3d_motion;      // if rgb=FALSE, LED value sets Z displacement
  // is_led_fixed_stacking applies only when rgb=FALSE:
  // 0: LED Z displacement is relative to prev LED
  // 1: LED Z displacement starts at 0,1,2;  2: LED Z displacement starts at 0
  int is_led_fixed_stacking;
  uint32_t dumpmask;

public:
  DebugLayer(Args* args, SpatialComputer* parent);
  void add_device(Device* d);
  BOOL handle_key(KeyEvent* event);
  // hardware emulation
  void set_probe (DATA* val, uint8_t index); // debugging data probe
  void dump_header(FILE* out); // list log-file fields
 private:
  void leds_op(MACHINE* machine);
  void red_op(MACHINE* machine);
  void green_op(MACHINE* machine);
  void blue_op(MACHINE* machine);
  void rgb_op(MACHINE* machine);
  void hsv_op(MACHINE* machine);
  void sense_op(MACHINE* machine);
  void set_r_led (NUM_VAL val);
  void set_g_led (NUM_VAL val);
  void set_b_led (NUM_VAL val);
  NUM_VAL read_sensor (uint8_t n); // for "user" sensors
};

class DebugDevice : public DeviceLayer {
  DebugLayer* parent;
public:
  DATA probes[MAX_PROBES];          // debugging probes
  DebugDevice(DebugLayer* parent, Device* container);
  void preupdate();
  void visualize();
  BOOL handle_key(KeyEvent* event);
  void copy_state(DeviceLayer* src) {} // to be called during cloning
  void dump_state(FILE* out, int verbosity); // print state to file
};

/*****************************************************************************
 *  PERFECT LOCALIZER                                                        *
 *****************************************************************************/
class PerfectLocalizer : public Layer, public HardwarePatch {
    
 public:
  PerfectLocalizer(SpatialComputer* parent);
  void add_device(Device* d);
  NUM_VAL read_speed (VOID);

  // returns a list of function  that it patches/ provides impementation for
  static vector<HardwareFunction> getImplementedHardwareFunctions();
 private:
  void coord_op(MACHINE* machine);
  VEC_VAL* read_coord_sensor(VOID);
};
class PerfectLocalizerDevice : public DeviceLayer {
 public:
  DATA* coord_sense; // data location for kernel to access coordinates
  PerfectLocalizerDevice(Device* container) : DeviceLayer(container) 
    { coord_sense = NULL; }
  void copy_state(DeviceLayer*) {} // no state worth copying
};

class LeftoverLayer : public Layer {
 public:
  LeftoverLayer(SpatialComputer* parent);
 private:
  void ranger_op(MACHINE* machine);
  void mouse_op(MACHINE* machine);
  void local_fold_op(MACHINE* machine);
  void fold_complete_op(MACHINE* machine);
  void channel_op(MACHINE* machine);
  void drip_op(MACHINE* machine);
  void concentration_op(MACHINE* machine);
  void channel_grad_op(MACHINE* machine);
  void cam_op(MACHINE* machine);

  VEC_VAL* read_ranger();
  VEC_VAL* read_mouse_sensor();
  BOOL set_is_folding(int n, int k);
  BOOL read_fold_complete(int n);
  NUM_VAL set_channel(int n, int k);
  NUM_VAL drip_channel(int n, int k);
  NUM_VAL read_channel(int n);
  VEC_VAL* grad_channel(int n);
  NUM_VAL cam_get(int n);

  void hardware_error(const char* name) {
    uerror("Attempt to use unimplemented hardware function '%s'",name);
  }
};

#endif // __BASIC_HARDWARE__


/* STILL UNHANDLED HARDWARE
  virtual NUM_VAL cam_get (int k) { hardware_error("cam_get"); }
  
  virtual void set_is_folding (BOOL val, int k) 
  { hardware_error("set_is_folding"); }
  virtual BOOL read_fold_complete (int val) 
  { hardware_error("read_fold_complete"); }
  
  virtual NUM_VAL set_channel (NUM_VAL diffusion, int k) 
  { hardware_error("set_channel"); }
  virtual NUM_VAL read_channel (int k) { hardware_error("read_channel"); }
  virtual NUM_VAL drip_channel (NUM_VAL val, int k) 
  { hardware_error("drip_channel"); }
  virtual VEC_VAL *grad_channel (int k) { hardware_error("grad_channel"); }
*/
