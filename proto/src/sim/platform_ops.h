/* Opcodes that customize the proto kernel for the simulator platform
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// Kernel expansions:
typedef enum {
  // Sensor and Actuator Ops  
  DIE_OP = CORE_CMD_OPS,
  CLONE_OP,
  COORD_OP,
  RANGER_OP,
  SENSE_OP,
  BUTTON_OP,
  SLIDER_OP,
  LIGHT_OP,
  SOUND_OP,
  SPEAK_OP,
  TEMP_OP,
  MOUSE_OP,
  CONDUCTIVE_OP,
  LOCAL_FOLD_OP,
  FOLD_COMPLETE_OP,
  LEDS_OP,
  RED_OP,
  GREEN_OP,
  BLUE_OP,
  RGB_OP,
  HSV_OP,
  RADIUS_SET_OP,
  RADIUS_OP,
  BUMP_OP,
  CHANNEL_OP,
  DRIP_OP,
  CONCENTRATION_OP,
  CHANNEL_GRAD_OP,
  CAM_OP,
  MAX_CMD_OPS
} PLATFORM_OPCODES;
