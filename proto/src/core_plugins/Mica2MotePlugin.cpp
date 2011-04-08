/* Plugin providing emulation of some common I/O devices on Mica2 Motes
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <sstream>
#include "config.h"
#include "Mica2MotePlugin.h"
#include "proto_vm.h"
#include "visualizer.h"
using namespace std;

#define SPEAK_OP "speak scalar scalar"
#define LIGHT_OP "light scalar"
#define SOUND_OP "sound scalar"
#define TEMP_OP "temp scalar"
#define CONDUCTIVE_OP "conductive scalar"
#define BUTTON_OP "button scalar scalar"
#define SLIDER_OP "slider scalar scalar scalar scalar scalar scalar scalar"

/*****************************************************************************
 *  TESTBED MOTE IO                                                          *
 *****************************************************************************/
MoteIO::MoteIO(Args* args, SpatialComputer* parent) : Layer(parent) {
  ensure_colors_registered("MoteIO");
  args->undefault(&can_dump,"-Dmoteio","-NDmoteio");
  // register patches
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::speak_op, SPEAK_OP));
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::light_op, LIGHT_OP));
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::sound_op, SOUND_OP));
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::temp_op, TEMP_OP));
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::conductive_op, CONDUCTIVE_OP));
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::button_op, BUTTON_OP));
  parent->hardware.registerOpcode(new OpHandler<MoteIO>(this, &MoteIO::slider_op, SLIDER_OP));
}

Color* MoteIO::BUTTON_COLOR;
void MoteIO::register_colors() {
#ifdef WANT_GLUT
  BUTTON_COLOR = palette->register_color("BUTTON_COLOR", 0, 1.0, 0.5, 0.8);
#endif
}

void MoteIO::speak_op(MACHINE* machine) {
  set_speak(NUM_PEEK(0));
}

void MoteIO::light_op(MACHINE* machine) {
  NUM_PUSH(read_light_sensor());
}

void MoteIO::sound_op(MACHINE* machine) {
  NUM_PUSH(read_microphone());
}

void MoteIO::temp_op(MACHINE* machine) {
  NUM_PUSH(read_temp());
}

void MoteIO::conductive_op(MACHINE* machine) {
  NUM_PUSH(read_short());
}

void MoteIO::button_op(MACHINE* machine) {
  NUM_PUSH(read_button((int) NUM_POP()));
}

void MoteIO::slider_op(MACHINE* machine) {
  int     dkey = (int)NUM_PEEK(5);
  int     ikey = (int)NUM_PEEK(4);
  NUM_VAL init = NUM_PEEK(3);
  NUM_VAL incr = NUM_PEEK(2);
  NUM_VAL min  = NUM_PEEK(1);
  NUM_VAL max  = NUM_PEEK(0);
  NPOP(6); NUM_PUSH(read_slider(dkey, ikey, init, incr, min, max));
}

void MoteIO::add_device(Device* d) {
  d->layers[id] = new DeviceMoteIO(this,d);
}
BOOL MoteIO::handle_key(KeyEvent* event) {
  return FALSE; // right now, there's no keys that affect these globally
}

void MoteIO::dump_header(FILE* out) {
  if(can_dump) fprintf(out," \"SOUND\" \"TEMP\" \"BUTTON\"");
}

// hardware emulation
void MoteIO::set_speak (NUM_VAL period) {
  // right now, setting the speaker does *nothing*, as in the old sim
}
NUM_VAL MoteIO::read_light_sensor(VOID) { return machine->sensors[LIGHT]; }
NUM_VAL MoteIO::read_microphone (VOID) { return machine->sensors[SOUND]; }
NUM_VAL MoteIO::read_temp (VOID) { return machine->sensors[TEMPERATURE]; }
NUM_VAL MoteIO::read_short (VOID) { return 0; }
NUM_VAL MoteIO::read_button (uint8_t n) {
  return ((DeviceMoteIO*)device->layers[id])->button;
}
NUM_VAL MoteIO::read_slider (uint8_t ikey, uint8_t dkey, NUM_VAL init, 
			     NUM_VAL incr, NUM_VAL min, NUM_VAL max) {
  // slider is not yet implemented
}

void DeviceMoteIO::dump_state(FILE* out, int verbosity) {
  MACHINE* m = container->vm;
  if(verbosity==0) { 
    fprintf(out," %.2f %.2f %d", m->sensors[SOUND], m->sensors[TEMPERATURE], 
            button);
  } else { 
    fprintf(out,"Mic = %.2f, Temp = %.2f, Button = %s\n",m->sensors[SOUND], 
            m->sensors[TEMPERATURE], bool2str(button));
  }
}

// individual device implementations
BOOL DeviceMoteIO::handle_key(KeyEvent* key) {
  // I think that the slider is supposed to consume keys too
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'N': button = !button; return TRUE;
    }
  }
  return FALSE;
}

#define SENSOR_RADIUS_FACTOR 4
void DeviceMoteIO::visualize(Device* d) {
#ifdef WANT_GLUT
  flo rad = d->body->display_radius();
  // draw the button being on
  if (button) {
    palette->use_color(MoteIO::BUTTON_COLOR);
    draw_disk(rad*SENSOR_RADIUS_FACTOR);
  }
#endif // WANT_GLUT
}



/*************** Plugin Library ***************/
void* Mica2MotePlugin::get_sim_plugin(string type, string name, Args* args, 
                                      SpatialComputer* cpu, int n) {
  if(type==LAYER_PLUGIN) {
    if(name==MICA2MOTE_NAME) { return new MoteIO(args,cpu); }
  }
  return NULL;
}

string Mica2MotePlugin::inventory() {
  return "# Some types of Mica2 Mote I/O\n" +
    registry_entry(LAYER_PLUGIN,MICA2MOTE_NAME,DLL_NAME);
}

extern "C" {
  ProtoPluginLibrary* get_proto_plugin_library()
  { return new Mica2MotePlugin(); }
  const char* get_proto_plugin_inventory()
  { return (new string(Mica2MotePlugin::inventory()))->c_str(); }
}
