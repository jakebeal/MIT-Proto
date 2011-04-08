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

/// PluginTypeInventory is a map from plugin name -> containing library name
typedef map<string,string> PluginTypeInventory;
/// PluginInventory is a map from plugin type -> type inventory
typedef map<string, PluginTypeInventory> PluginInventory;
/// LibraryCollcetion is a map from open library name -> object
typedef map<string, ProtoPluginLibrary*> LibraryCollection;

class ProtoPluginManager {
 public:
  ProtoPluginManager();
  
  // Getters return null if they fail, and as a side effect print failure msgs
  // MUST CHECK FOR NULL ON RETURN
  // Used to get simulator Layers, Distributions, and TimeModels
  void* get_sim_plugin(string type, string name, Args* args, 
                       SpatialComputer* cpu, int n);
  // Used to get compiler extensions, or just the opcode part of a sim plugin
  void* get_compiler_plugin(string type, string name, Args* args, Compiler* c);
  
  // Accessor for get_plugin_inventory
  const PluginInventory* get_plugin_inventory();

  static string const PLUGIN_DIR; // Where DLLs go
  static string const REGISTRY_FILE_NAME; // Where the registry goes
  static string const PLATFORM_DIR; // Where .proto extension files go
  static string const PLATFORM_OPFILE; // The default .proto file for a platform

  void register_lib(string type,string name,string key,ProtoPluginLibrary* lib);
  void ensure_initialized(Args* args);

 private:
  bool initialized;
  // TODO: typdef these maps
  // e.g. to "RegistryEntry" or something and document it with autodoc
  PluginInventory registry;
  LibraryCollection open_libs;
  ProtoPluginLibrary* get_plugin_lib(string type, string name,Args* args);
  bool read_dll(string libfile);
  bool read_registry_file();
  bool parse_registry(istream &reg,string overridename="");
};

extern ProtoPluginManager plugins; // global manager, initializes on first use

#endif // __PLUGIN_MANAGER__
