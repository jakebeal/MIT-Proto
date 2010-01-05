/* Common functionality for most radio simulations.

Copyright (C) 2009, Nelson Elhage and contributors listed in the
AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory.  */

#include "config.h"
#include "radio.h"

RadioSim::RadioSim(Args* args, SpatialComputer* p) : Layer(p) {
  tx_error = (args->extract_switch("-txerr"))?args->pop_number():0.0;
  rx_error = (args->extract_switch("-rxerr"))?args->pop_number():0.0;

  is_show_connectivity = args->extract_switch("-c");
  connect_display_mode = 
    (args->extract_switch("-sharp-connections")) ? 2 :
    (args->extract_switch("-sharp-neighborhood")) ? 1 : 0;
}

RadioSim::~RadioSim() {
  
}

BOOL RadioSim::handle_key(KeyEvent* key) {
  if(key->normal) {
    if(!key->ctrl) {
      switch(key->key) {
      case 'c': is_show_connectivity = !is_show_connectivity; return TRUE;
      case 'C': connect_display_mode = (connect_display_mode+1)%3; return TRUE;
      case 'S': is_show_backoff = !is_show_backoff; return TRUE;
      }
    }
  }
  return FALSE;
}

BOOL RadioSim::try_tx() {
  return tx_error==0 || urnd(0,1) > tx_error;
}

BOOL RadioSim::try_rx() {
  return rx_error==0 || urnd(0,1) > rx_error;
}
