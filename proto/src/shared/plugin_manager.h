/* Mechanisms for handling plugin registry and loading
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __PLUGIN_MANAGER__
#define __PLUGIN_MANAGER__

#include <map>
#include "utils.h"
#include "proto_plugin.h"

class ProtoPluginManager {
 public:
  ProtoPluginManager();

  // Getters return null if they fail, and as a side effect print failure msgs
  // Used to get simulator Layers, Distributions, and TimeModels
  void* get_sim_plugin(string type, string name, Args* args, 
                       SpatialComputer* cpu, int n);
  // Used to get compiler extensions, or just the opcode part of a sim plugin
  void* get_compiler_plugin(string type, string name, Args* args);
  // Accessor for get_plugin_inventory: NO NOT USE THIS TO MODIFY IT!
  map<string, map<string,string> >* get_plugin_inventory() 
    { ensure_initialized(); return &registry; }

  static string PLUGIN_DIR;
  static string REGISTRY_FILE_NAME;

  void register_lib(string type,string name,string key,ProtoPluginLibrary* lib)
  { registry[type][name] = key; open_libs[key] = lib; }

 private:
  bool initialized;
  map<string, map<string,string> > registry; // type -> name -> library
  map<string, ProtoPluginLibrary*> open_libs; // open library name -> object
  void ensure_initialized();
  bool read_registry();
};

extern ProtoPluginManager plugins; // global manager, initializes on first use

#endif // __PLUGIN_MANAGER__
