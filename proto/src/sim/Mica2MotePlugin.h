/* Plugin providing emulation of some common I/O devices on Mica2 Motes
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef _MICA2MOTEPLUGIN_
#define	_MICA2MOTEPLUGIN_

#include "proto_plugin.h"
#include "spatialcomputer.h"

/*****************************************************************************
 *  TESTBED MOTE IO                                                          *
 *****************************************************************************/
class MoteIO : public Layer, public HardwarePatch {
 public:
  MoteIO(Args* args, SpatialComputer* parent);
  void add_device(Device* d);
  BOOL handle_key(KeyEvent* event);
  void dump_header(FILE* out); // list log-file fields
 private:
  void speak_op(MACHINE* machine);
  void light_op(MACHINE* machine);
  void sound_op(MACHINE* machine);
  void temp_op(MACHINE* machine);
  void conductive_op(MACHINE* machine);
  void button_op(MACHINE* machine);
  void slider_op(MACHINE* machine);

  // hardware emulation
  void set_speak (NUM_VAL period);
  NUM_VAL read_light_sensor(VOID);
  NUM_VAL read_microphone (VOID);
  NUM_VAL read_temp (VOID);
  NUM_VAL read_short (VOID);       // test for conductivity
  NUM_VAL read_button (uint8_t n);
  NUM_VAL read_slider (uint8_t ikey, uint8_t dkey, NUM_VAL init, // frob knob
		       NUM_VAL incr, NUM_VAL min, NUM_VAL max);
};

class DeviceMoteIO : public DeviceLayer {
  MoteIO* parent;
 public:
  BOOL button;
  DeviceMoteIO(MoteIO* parent, Device* d) : DeviceLayer(d)
    { this->parent=parent; button=FALSE; }
  void visualize(Device* d);
  BOOL handle_key(KeyEvent* event);
  void copy_state(DeviceLayer* src) {} // to be called during cloning
  void dump_state(FILE* out, int verbosity); // print state to file
};

// Plugin class
#define MICA2MOTE_NAME "mote-io"
#define DLL_NAME "libmica2mote"
class Mica2MotePlugin : public ProtoPluginLibrary {
public:
  void* get_sim_plugin(string type, string name, Args* args, 
                       SpatialComputer* cpu, int n);
  static string inventory();
};

#endif	// _MICA2MOTEPLUGIN_

