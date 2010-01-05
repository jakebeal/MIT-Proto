/* Common functionality for most radio simulations.

Copyright (C) 2009, Nelson Elhage and contributors listed in the
AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory.  */

#ifndef  __RADIOSIM__
#define  __RADIOSIM__

#include "spatialcomputer.h"

class RadioSim : public Layer, public HardwarePatch {
  public:
  // model options
  float tx_error;            // probability of failure on transmit
  float rx_error;            // probability of failure before reception
  // display options
  BOOL is_show_connectivity;
  BOOL is_show_backoff;
  int connect_display_mode; // fuzzy=0, locally sharp=1, or sharp=2

  RadioSim(Args *args, SpatialComputer *parent);
  ~RadioSim();

  BOOL handle_key(KeyEvent* key);

protected:
  BOOL try_tx();
  BOOL try_rx();
};

#endif
