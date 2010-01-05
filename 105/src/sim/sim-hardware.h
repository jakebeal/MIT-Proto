/* Code to manage the interface between the simulator and the kernel.
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __SIM_HARDWARE__
#define __SIM_HARDWARE__

#include "proto.h"

// posts data without pretty nesting
void post_stripped_data_to (char *str, DATA *d);

// HardwarePatch is a class that Layers should extend in order to make
// their dynamics available to the kernel.
class HardwarePatch {
public:
  void hardware_error(char* name) {
    uerror("Attempt to use unimplemented hardware function '%s'",name);
  }

  virtual void mov (VEC_VAL *val) { hardware_error("mov"); }
  virtual void flex (NUM_VAL val) { hardware_error("flex"); }
  virtual void die (NUM_VAL val) { hardware_error("die"); }
  virtual void clone_machine (NUM_VAL val) { hardware_error("clone_machine"); }
  
  virtual NUM_VAL cam_get (int k) { hardware_error("cam_get"); }
  virtual NUM_VAL radius_get (VOID) { hardware_error("radius_get"); }
  virtual NUM_VAL radius_set (NUM_VAL val) { hardware_error("radius_set"); }
  
  virtual void set_r_led (NUM_VAL val) { hardware_error("set_r_led"); }
  virtual void set_g_led (NUM_VAL val) { hardware_error("set_g_led"); }
  virtual void set_b_led (NUM_VAL val) { hardware_error("set_b_led"); }
  virtual void set_probe (DATA* d, uint8_t p) { hardware_error("set_probe"); }
  virtual void set_speak (NUM_VAL period) { hardware_error("set_speak"); }
  
  virtual NUM_VAL set_dt (NUM_VAL dt) { hardware_error("set_dt"); }

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
  
  virtual NUM_VAL read_radio_range (VOID) {hardware_error("read_radio_range");}
  virtual NUM_VAL read_light_sensor(VOID){hardware_error("read_light_sensor");}
  virtual NUM_VAL read_microphone (VOID) { hardware_error("read_microphone"); }
  virtual NUM_VAL read_temp (VOID) { hardware_error("read_temp"); }
  virtual NUM_VAL read_short (VOID) { hardware_error("read_short"); }
  virtual NUM_VAL read_sensor (uint8_t n) { hardware_error("read_sensor"); }
  virtual VEC_VAL *read_coord_sensor(VOID)
  { hardware_error("read_coord_sensor"); }
  virtual VEC_VAL *read_mouse_sensor(VOID)
  { hardware_error("read_mouse_sensor"); }
  virtual VEC_VAL *read_ranger (VOID) { hardware_error("read_ranger"); }
  virtual NUM_VAL read_bearing (VOID) { hardware_error("read_bearing"); }
  virtual NUM_VAL read_speed (VOID) { hardware_error("read_speed"); }
  virtual NUM_VAL read_bump (VOID) { hardware_error("read_bump"); }
  virtual NUM_VAL read_button (uint8_t n) { hardware_error("read_button"); }
  virtual NUM_VAL read_slider (uint8_t ikey, uint8_t dkey, NUM_VAL init, 
                               NUM_VAL incr, NUM_VAL min, NUM_VAL max) 
  { hardware_error("read_slider"); }
  
  virtual int radio_send_export (uint8_t version, uint8_t timeout, uint8_t n, 
                                  uint8_t len, COM_DATA *buf) 
  { hardware_error("radio_send_export"); }
  virtual int radio_send_script_pkt (uint8_t version, uint16_t n, 
                                      uint8_t pkt_num, uint8_t *script) 
  { hardware_error("radio_send_script_pkt"); }
  virtual int radio_send_digest (uint8_t version, uint16_t script_len, 
                                 uint8_t *digest) 
  { hardware_error("radio_send_digest"); }
};

// A list of all the functions that can be supplied with a HardwarePatch,
// for use in applying the patches in a SimulatedHardware
enum HardwareFunction {
  MOV_FN, FLEX_FN, DIE_FN, CLONE_MACHINE_FN, 
  CAM_GET_FN, RADIUS_GET_FN, RADIUS_SET_FN, 
  SET_R_LED_FN, SET_G_LED_FN, SET_B_LED_FN, SET_PROBE_FN, SET_SPEAK_FN, 
  SET_DT_FN, SET_IS_FOLDING_FN, READ_FOLD_COMPLETE_FN, 
  SET_CHANNEL_FN, READ_CHANNEL_FN, DRIP_CHANNEL_FN, GRAD_CHANNEL_FN,
  READ_RADIO_RANGE_FN, READ_LIGHT_SENSOR_FN, READ_MICROPHONE_FN, 
  READ_TEMP_FN, READ_SHORT_FN, READ_SENSOR_FN, READ_COORD_SENSOR_FN, 
  READ_MOUSE_SENSOR_FN, READ_RANGER_FN, READ_BEARING_FN, READ_SPEED_FN, 
  READ_BUMP_FN, READ_BUTTON_FN, READ_SLIDER_FN, 
  RADIO_SEND_EXPORT_FN, RADIO_SEND_SCRIPT_PKT_FN, RADIO_SEND_DIGEST_FN,
  NUM_HARDWARE_FNS
};

// This class dispatches kernel hardware calls to the appropriate patches
class Device;
class SimulatedHardware {
public:
  BOOL is_kernel_debug, is_kernel_trace, is_kernel_debug_script;
  HardwarePatch base;
  HardwarePatch* patch_table[NUM_HARDWARE_FNS];
  
  SimulatedHardware();
  void patch(HardwarePatch* p, HardwareFunction fn); // instantiate a fn
  void set_vm_context(Device* d); // prepare globals for kernel execution
};

// globals that carry the VM context for kernel hardware calls
// HardwarePatch classes can count on them being set to correct values
extern SimulatedHardware* hardware;
extern Device* device;

// Construction and destruction within a device object
extern MACHINE* allocate_machine();
extern void deallocate_machine(MACHINE** vm);

#endif //__SIM_HARDWARE__
