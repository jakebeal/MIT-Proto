/* A collection of simple common hardware packages for the simulator
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include "basic-hardware.h"
#include "visualizer.h"
#include "proto_vm.h"

/*****************************************************************************
 *  DEBUG                                                                    *
 *****************************************************************************/
DebugLayer::DebugLayer(Args* args, SpatialComputer* p) : Layer(p) {
  // pull display options
  n_probes = args->extract_switch("-probes") ? (int)args->pop_number():0;
  is_show_leds = args->extract_switch("-l");
  is_led_rgb = args->extract_switch("-led-blend");
  is_led_ghost_mode = args->extract_switch("-led-ghost");
  is_led_3d_motion = !args->extract_switch("-led-flat");
  is_led_fixed_stacking = 
    (args->extract_switch("-led-stacking")) ? (int)args->pop_number():0;
  args->undefault(&can_dump,"-Ddebug","-NDdebug");
  dumpmask = args->extract_switch("-Ddebug-mask") ? (int)args->pop_number():-1;
  // register patches
  parent->hardware.patch(this,SET_PROBE_FN);
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::leds_op,  "leds scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::red_op,   "red scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::green_op, "green scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::blue_op,  "blue scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::rgb_op,   "rgb (vector 3) (vector 3)"));
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::hsv_op,   "hsv (vector 3) (vector 3)"));
  parent->hardware.registerOpcode(new OpHandler<DebugLayer>(this, &DebugLayer::sense_op, "sense scalar scalar"));
}

void DebugLayer::leds_op(MACHINE* machine) {
  NUM_VAL val = NUM_PEEK(0);
  set_b_led((val > 0.25) != 0 ? 1.0 : 0);
  set_g_led((val > 0.50) != 0 ? 1.0 : 0);
  set_r_led((val > 0.75) != 0 ? 1.0 : 0);
}

void DebugLayer::red_op(MACHINE* machine) {
  NUM_VAL val = NUM_PEEK(0);
  set_r_led(val);
}

void DebugLayer::green_op(MACHINE* machine) {
  NUM_VAL val = NUM_PEEK(0);
  set_g_led(val);
}

void DebugLayer::blue_op(MACHINE* machine) {
  NUM_VAL val = NUM_PEEK(0);
  set_b_led(val);
}

void DebugLayer::rgb_op(MACHINE* machine) {
  VEC_VAL *vec = VEC_PEEK(0);
  set_r_led(num_vec_elt(vec, 0));
  set_g_led(num_vec_elt(vec, 1));
  set_b_led(num_vec_elt(vec, 2));
}

void DebugLayer::hsv_op(MACHINE* machine) {
  int off = NXT_OP(machine);
  VEC_VAL *rgb = VEC_GET(GLO_GET(off));
  VEC_VAL *hsv = VEC_PEEK(0);
  flo r, g, b;
  hsv_to_rgb(num_vec_elt(hsv, 0), num_vec_elt(hsv, 1), num_vec_elt(hsv, 2), &r, &g, &b);
  num_vec_elt_set(rgb, 0, r);
  num_vec_elt_set(rgb, 1, g);
  num_vec_elt_set(rgb, 2, b);
  NPOP(1); VEC_PUSH(rgb);
}

void DebugLayer::sense_op(MACHINE* machine) {
  NUM_PUSH(read_sensor((uint8_t) NUM_POP()));
}  

BOOL DebugLayer::handle_key(KeyEvent* key) {
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'p': n_probes = (n_probes + 1)%(MAX_PROBES+1); return TRUE;
    case 'L': is_show_leds = !is_show_leds; return TRUE;
    case '1': is_led_ghost_mode = !is_led_ghost_mode; return TRUE;
    case '2': is_led_3d_motion = !is_led_3d_motion; return TRUE;
    case '3': is_led_fixed_stacking = (is_led_fixed_stacking+1)%3; return TRUE;
    case '4': is_led_rgb = !is_led_rgb; return TRUE;
    }
  }
  return FALSE;
}
void DebugLayer::add_device(Device* d) {
  d->layers[id] = new DebugDevice(this,d);
}

void DebugLayer::dump_header(FILE* out) {
  if(can_dump) {
    // there was another sensor here, but it's been removed
    if(dumpmask & 0x02) fprintf(out," \"USER1\"");
    if(dumpmask & 0x04) fprintf(out," \"USER2\"");
    if(dumpmask & 0x08) fprintf(out," \"USER3\"");
    if(dumpmask & 0x10) fprintf(out," \"R_LED\"");
    if(dumpmask & 0x20) fprintf(out," \"G_LED\"");
    if(dumpmask & 0x40) fprintf(out," \"B_LED\"");
  }
}

// actuators
void DebugLayer::set_probe (DATA* val, uint8_t index) { 
  if(index >= MAX_PROBES) return; // sanity check index
  DATA_SET(&((DebugDevice*)device->layers[id])->probes[index], val); 
}
void DebugLayer::set_r_led (NUM_VAL val) { machine->actuators[R_LED] = val; }
void DebugLayer::set_g_led (NUM_VAL val) { machine->actuators[G_LED] = val; }
void DebugLayer::set_b_led (NUM_VAL val) { machine->actuators[B_LED] = val; }
NUM_VAL DebugLayer::read_sensor (uint8_t n) {
  return (n<N_SENSORS) ? machine->sensors[n] : NAN;
}

// per-device interface, used primarily for visualization
DebugDevice::DebugDevice(DebugLayer* parent, Device* d) : DeviceLayer(d) {
  for(int i=0;i<MAX_PROBES;i++) NUM_SET(&probes[i], 0); // probes start clear
  this->parent = parent;
}

void DebugDevice::dump_state(FILE* out, int verbosity) {
  MACHINE* m = container->vm;
  if(verbosity==0) {
    uint32_t dumpmask = parent->dumpmask; // shorten the name
    // there was another sensor here, but it's been removed
    if(dumpmask & 0x02) fprintf(out," %.2f",m->sensors[1]);
    if(dumpmask & 0x04) fprintf(out," %.2f",m->sensors[2]);
    if(dumpmask & 0x08) fprintf(out," %.2f",m->sensors[3]);
    if(dumpmask & 0x10) fprintf(out," %.3f",m->actuators[R_LED]);
    if(dumpmask & 0x20) fprintf(out," %.3f",m->actuators[G_LED]);
    if(dumpmask & 0x40) fprintf(out," %.3f",m->actuators[B_LED]);
    // probes can't be output gracefully since we don't know what they contain
  } else {
    fprintf(out,"Sensors: User-1=%.2f User-2=%.2f User-3=%.2f\n",
            m->sensors[1], m->sensors[2], m->sensors[3]);
    fprintf(out,"LEDs: R=%.3f G=%.3f B=%.3f\n", m->actuators[R_LED],
            m->actuators[G_LED], m->actuators[B_LED]);
    fprintf(out,"Probes:");
    char buf[1000];
    for(int i=0;i<MAX_PROBES;i++) {
      post_data_to(buf,&probes[i]); fprintf(out,"%s ",buf);
    }
    fprintf(out,"\n");
  }
}

BOOL DebugDevice::handle_key(KeyEvent* key) {
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 't': 
      container->vm->sensors[1] = container->vm->sensors[1] ? 0:1; return TRUE;
    case 'y': 
      container->vm->sensors[2] = container->vm->sensors[2] ? 0:1; return TRUE;
    case 'u': 
      container->vm->sensors[3] = container->vm->sensors[3] ? 0:1; return TRUE;
    }
  }
  return FALSE;
}

void DebugDevice::preupdate() {
  for(int i=0;i<MAX_PROBES;i++) { NUM_SET(&probes[i],0); }
}

#define SENSOR_RADIUS_FACTOR 4
void DebugDevice::visualize() {
#ifdef WANT_GLUT
  MACHINE* vm = container->vm;
  static ColorName user[3] = {USER_SENSOR_1, USER_SENSOR_2, USER_SENSOR_3};
  flo rad = container->body->display_radius();
  // draw user sensors
  for(int i=0;i<3;i++) {
    if(vm->sensors[i+1] > 0) { 
      palette->use_color(user[i]);
      draw_disk(rad*SENSOR_RADIUS_FACTOR);
    }
  }
  // draw LEDs
  if (parent->is_show_leds) {
    static ColorName led_color[3] = {RED_LED, GREEN_LED, BLUE_LED};
    flo led[3] = { vm->actuators[R_LED], vm->actuators[G_LED],
		   vm->actuators[B_LED] };
    glPushMatrix();
    if (parent->is_led_rgb) {
      if (led[0] || led[1] || led[2]) {
	palette->scale_color(RGB_LED, led[0],led[1],led[2],1);
        draw_disk(rad*2); // double size because the legacy code sez so
      }
    } else {
      for(int i=0;i<3;i++) {
	if(led[i]==0) continue;
        if(parent->is_led_fixed_stacking) { 
	  glPushMatrix();
	  if(parent->is_led_fixed_stacking==1) glTranslatef(0,0,i); 
	}
        if(parent->is_led_ghost_mode)
	  palette->scale_color(led_color[i],1,1,1,led[i]);
	else
	  palette->scale_color(led_color[i],led[i],led[i],led[i],1);
        if(parent->is_led_3d_motion) glTranslatef(0,0,led[i]);
        draw_disk(rad); // actually draw the damned thing
        if(parent->is_led_fixed_stacking) { glPopMatrix(); }
      }
    }
    glPopMatrix();
  }
  // draw probes
  if (parent->n_probes > 0) {
    glPushMatrix();
    container->text_scale(); // prepare to draw text
    char buf[1024];
    glTranslatef(0, 0.5625, 0);
    for (int i = 0; i < parent->n_probes; i++) {
      post_data_to(buf, &probes[i]);
      palette->use_color(DEVICE_PROBES);
      draw_text(1, 1, buf);
      glTranslatef(1.125, 0, 0);
    }
    glPopMatrix();
  }
#endif // WANT_GLUT
}



/*****************************************************************************
 *  PERFECT LOCALIZER                                                        *
 *****************************************************************************/
PerfectLocalizer::PerfectLocalizer(SpatialComputer* parent) : Layer(parent) {
  parent->hardware.registerOpcode(new OpHandler<PerfectLocalizer>(this, &PerfectLocalizer::coord_op, "coord (vector 3)"));
  parent->hardware.patch(this,READ_SPEED_FN);
}

void PerfectLocalizer::coord_op(MACHINE* machine) {
  VEC_PUSH(read_coord_sensor());
}

vector<HardwareFunction> PerfectLocalizer::getImplementedHardwareFunctions()
{
    vector<HardwareFunction> hardwareFunctions;
    hardwareFunctions.push_back(READ_SPEED_FN);
    return hardwareFunctions;
}

void PerfectLocalizer::add_device(Device* d) {
  d->layers[id] = new PerfectLocalizerDevice(d);
}

VEC_VAL* PerfectLocalizer::read_coord_sensor(VOID) {
  PerfectLocalizerDevice* d = (PerfectLocalizerDevice*)device->layers[id];
  if(d->coord_sense==NULL) {
    DATA num; d->coord_sense = new_tup(3, init_num(&num, 0.0));
  }
  const METERS* p = device->body->position();
  for(int i=0;i<3;i++) NUM_SET(&VEC_GET(d->coord_sense)->elts[i], p[i]);
  return VEC_GET(d->coord_sense);
}

NUM_VAL PerfectLocalizer::read_speed(VOID) {
  const METERS* v = device->body->velocity();
  return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

LeftoverLayer::LeftoverLayer(SpatialComputer* parent) : Layer(parent) {
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::ranger_op, "ranger (vector 3)"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::mouse_op, "mouse (vector 3)"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::local_fold_op, "local-fold scalar scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::fold_complete_op, "fold-complete scalar boolean"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::channel_op, "new-channel scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::drip_op, "drip scalar scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::concentration_op, "concentration scalar scalar"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::channel_grad_op, "channel-grad (vector 3) scalar"));
  parent->hardware.registerOpcode(new OpHandler<LeftoverLayer>(this, &LeftoverLayer::cam_op, "cam scalar scalar"));
}

void LeftoverLayer::ranger_op(MACHINE* machine) {
  VEC_PUSH(read_ranger());
}

void LeftoverLayer::mouse_op(MACHINE* machine) {
  VEC_PUSH(read_mouse_sensor());
}

void LeftoverLayer::local_fold_op(MACHINE* machine) {
  int k = (int)NUM_PEEK(0);
  int val = (BOOL)NUM_PEEK(1);
  set_is_folding((int) NUM_PEEK(1), k);
  NPOP(2);
  NUM_PUSH(val);
}

void LeftoverLayer::fold_complete_op(MACHINE* machine) {
  int k = (int) NUM_PEEK(0);
  NPOP(1);
  NUM_PUSH(read_fold_complete(k));
}

void LeftoverLayer::channel_op(MACHINE* machine) {
  int n = NXT_OP(machine);
  set_channel(NUM_POP(), n);
  NUM_PUSH(n);
}

void LeftoverLayer::drip_op(MACHINE* machine) {
  NUM_VAL a = NUM_PEEK(1);
  NUM_VAL c = NUM_PEEK(0);
  NPOP(2);
  NUM_PUSH(drip_channel(a, (int) c));
}

void LeftoverLayer::concentration_op(MACHINE* machine) {
  NUM_PUSH(read_channel((int) NUM_POP()));
}

void LeftoverLayer::channel_grad_op(MACHINE* machine) {
  VEC_PUSH(grad_channel((int) NUM_POP()));
}

void LeftoverLayer::cam_op(MACHINE* machine) {
  NUM_PUSH(cam_get((int) NUM_POP()));
}

VEC_VAL* LeftoverLayer::read_ranger() {
  hardware_error("read_ranger");
}

VEC_VAL* LeftoverLayer::read_mouse_sensor() {
  hardware_error("read_mouse_sensor");
}

BOOL LeftoverLayer::set_is_folding(int n, int k) {
  hardware_error("set_is_folding");
}

BOOL LeftoverLayer::read_fold_complete(int n) {
  hardware_error("read_fold_complete");
}

NUM_VAL LeftoverLayer::set_channel(int n, int k) {
  hardware_error("set_channel");
}

NUM_VAL LeftoverLayer::drip_channel(int n, int k) {
  hardware_error("drip_channel");
}

NUM_VAL LeftoverLayer::read_channel(int n) {
  hardware_error("drip_channel");
}

VEC_VAL* LeftoverLayer::grad_channel(int n) {
  hardware_error("drip_channel");
}

NUM_VAL LeftoverLayer::cam_get(int n) {
  hardware_error("drip_channel");
}
