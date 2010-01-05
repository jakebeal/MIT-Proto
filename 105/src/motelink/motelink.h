/* Dual-reality link for simulator to real devices
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// Right now, this is just a dummy class
#ifndef __MOTELINK__
#define __MOTELINK__

#include "spatialcomputer.h"
#include "visualizer.h"

// a proxy body may behave differently than an ordinary one
class ProxyBody : public Body {
  ProxyBody(Device* container) : Body(container) {}
};

/* class ProxyDevice : public Device { */
/*   ProxyDevice(SpatialComputer* parent, METERS *loc, DeviceTimer *timer) */
  
/*   void visualize(SpatialComputer* parent) { */
/*     palette->substitute_color(SIMPLE_BODY,MOTE_BODY); */
/*     Device::visualize(parent); */
/*     palette->pop_color(SIMPLE_BODY); */
/*   } */
/* }; */

class MoteLink : public EventConsumer {
 public:
  MoteLink(Args* args) {}
};

#endif // __MOTELINK__
