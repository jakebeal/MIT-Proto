/* Master header for creating plugins for Proto compiler and simulator
Copyright (C) 2005-2010, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __PROTO_PLUGIN__
#define __PROTO_PLUGIN__

#include "utils.h"
#include "compiler.h"

// We generic calls and type string-matching to make extensibility easier
#define LAYER_PLUGIN "Layer"
#define DISTRIBUTION_PLUGIN "Distribution"
#define TIMEMODEL_PLUGIN "TimeModel"
#define EMITTER_PLUGIN "Emitter"

class SpatialComputer; // compiler plugins may not know the content of SCs.

class ProtoPluginLibrary {
 public:
  // Used to get simulator Layers, Distributions, and TimeModels
  virtual void* get_sim_plugin(string type, string name, Args* args, 
                               SpatialComputer* cpu, int n) { return NULL; }
  // Used to get compiler extensions
  virtual void* get_compiler_plugin(string type, string name, Args* args,
                                    Compiler* c) 
    {return NULL; }
  static string registry_entry(string type,string name,string dll)
    { return type+" "+name+" = "+dll+"\n"; }
};

// Hook functions for external access to dynamically loaded library
extern "C" {
  ProtoPluginLibrary* get_proto_plugin_library();
  const char* get_proto_plugin_inventory();
}

// and the types for these functions
typedef const char* (*get_inventory_func)(void);
typedef ProtoPluginLibrary* (*get_library_func)(void);

#endif // __PROTO_PLUGIN__
