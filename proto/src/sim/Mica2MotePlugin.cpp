/* 
 * File:   Mica2MotePlugin.cpp
 * Author: prakash
 * 
 * Created on February 18, 2010, 5:35 PM
 */

#include "Mica2MotePlugin.h"
#include "Mica2Mote.h"
#include <sstream>
using namespace std;

void* Mica2MotePlugin::get_sim_plugin(string type, string name, Args* args, 
                                      SpatialComputer* cpu, int n) {
  if(type==LAYER_PLUGIN) {
    if(name==MICA2MOTE_NAME) { return new MoteIO(args,cpu); }
  }
}

void* Mica2MotePlugin::get_compiler_plugin(string type,string name,Args* args) {
  // TODO: implement compiler plugins
  uerror("Compiler plugins not yet implemented");
}

string Mica2MotePlugin::inventory() {
  return "# Some types of Mica2 Mote I/O\n" +
    registry_entry(LAYER_PLUGIN,MICA2MOTE_NAME,DLL_NAME);
}

extern "C" {
  ProtoPluginLibrary* get_proto_plugin_library()
  { return new Mica2MotePlugin(); }
  const char* get_proto_plugin_inventory()
  { return (new string(Mica2MotePlugin::inventory()))->c_str(); }
}
