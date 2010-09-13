/* A collection of simple distributions
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef _DISTRIBUTIONS_PLUGIN_
#define	_DISTRIBUTIONS_PLUGIN_

#include "proto_plugin.h"
#include "spatialcomputer.h"

class DistributionsPlugin : public ProtoPluginLibrary {
public:
  void* get_sim_plugin(string type, string name, Args* args,
                       SpatialComputer* cpu, int n);
  void* get_compiler_plugin(string type, string name, Args* args) {return NULL;}
  static string inventory();
};

#endif // _DISTRIBUTIONS_PLUGIN_

