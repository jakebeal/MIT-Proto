/* Simulator's extensions to the kernel
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include <stdlib.h>
#include "proto.h"
#include "proto_vm.h"
#include "sim-hardware.h"

#define CLIP(x, min, max) MAX(min, MIN(max, x))

static void hsv_to_rgb (flo h, flo s, flo v, flo *r, flo *g, flo *b) {
  flo rt = 0, gt = 0, bt = 0;
  s = CLIP(s, 0, 1);
  if (s == 0.0) {
    rt = gt = bt = v;
  } else {
    int i;
    flo h_temp = (h == 360.0) ? 0.0 : h;
    flo f, p, q, t; 
    h_temp /= 60.0;
    i = (int)h_temp;
    f = h_temp - i;
    p = v*(1-s);
    q = v*(1-(s*f));
    t = v*(1-(s*(1-f)));
    switch (i) {
    case 0: rt = v; gt = t; bt = p; break;
    case 1: rt = q; gt = v; bt = p; break;
    case 2: rt = p; gt = v; bt = t; break;
    case 3: rt = p; gt = q; bt = v; break;
    case 4: rt = t; gt = p; bt = v; break;
    case 5: rt = v; gt = p; bt = q; break;
    }
  }
  *r = rt; *g = gt; *b = bt;
}

void my_platform_operation(uint8_t op) {
  extern SimulatedHardware* hardware;
  hardware->dispatchOpcode(op);
}
