/* Programmed simulation termination

Copyright (C) 2009, Nelson Elhage and contributors listed in the
AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory.  */

#include "config.h"
#include "stop-when.h"
#include "visualizer.h"

StopWhen::StopWhen(Args *args, SpatialComputer *parent) : Layer(parent) {
  if(args->extract_switch("-stop-pct")) {
    this->stop_pct = args->pop_number();
  }
  this->n_devices = 0;

  parent->hardware.patch(this, SET_PROBE_FN);
}

StopWhen::~StopWhen() {
  
}

void StopWhen::add_device(Device *d) {
  n_devices++;
}

void StopWhen::set_probe (DATA *d, uint8_t p) {
  if(p == 0) {
    if(probed.find(device) == probed.end()) {
      probed.insert(device);
      if(probed.size() > n_devices * stop_pct
         || stop_pct == 1.0 && probed.size() == n_devices) {
        post("Stopping simulation at t=%f\n", parent->sim_time);
        exit(0);
      }
    }
  }
}

extern "C" Layer *get_layer(Args *args, SpatialComputer *cpu, int n);

Layer *get_layer(Args *args, SpatialComputer *cpu, int n) {
  return new StopWhen(args, cpu);
}
