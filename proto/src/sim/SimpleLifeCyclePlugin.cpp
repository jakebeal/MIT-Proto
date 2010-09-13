/* Plugin providing simple cloning and apoptosis model
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "SimpleLifeCyclePlugin.h"

void* SimpleLifeCyclePlugin::get_sim_plugin(string type,string name,Args* args, 
                                            SpatialComputer* cpu, int n) {
  if(type == LAYER_PLUGIN) {
    if(name == LAYER_NAME) { return new SimpleLifeCycle(args, cpu); }
  }
  return NULL;
}

void* SimpleLifeCyclePlugin::get_compiler_plugin(string type, string name, Args* args) {
  // TODO: implement compiler plugins
  uerror("Compiler plugins not yet implemented");
}

string SimpleLifeCyclePlugin::inventory() {
  return "# Cloning and apoptosis\n" +
    registry_entry(LAYER_PLUGIN,LAYER_NAME,DLL_NAME);
}

extern "C" {
  ProtoPluginLibrary* get_proto_plugin_library() 
  { return new SimpleLifeCyclePlugin(); }
  const char* get_proto_plugin_inventory()
  { return (new string(SimpleLifeCyclePlugin::inventory()))->c_str(); }
}
