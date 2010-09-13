/* Plugin providing emulation of some common I/O devices on Mica2 Motes
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef _MICA2MOTEPLUGIN_
#define	_MICA2MOTEPLUGIN_

#include "proto_plugin.h"

#define MICA2MOTE_NAME "mote-io"
#define DLL_NAME "libmica2mote"

// Plugin class
class Mica2MotePlugin : public ProtoPluginLibrary {
public:
  void* get_sim_plugin(string type, string name, Args* args, 
                       SpatialComputer* cpu, int n);
  void* get_compiler_plugin(string type, string name, Args* args);
  static string inventory();
};

#endif	// _MICA2MOTEPLUGIN_

