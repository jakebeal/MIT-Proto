/* Mechanisms for handling plugin registry and loading
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <ltdl.h>
#include <iostream>
#include <fstream>
#include <string>
#include "config.h"
#include "plugin_manager.h"

// Registry location:
string ProtoPluginManager::PLUGIN_DIR = string(PLUGINDIR);
string ProtoPluginManager::REGISTRY_FILE_NAME = "registry.txt";
string ProtoPluginManager::PLATFORM_DIR = string(PROTOPLATDIR);
string ProtoPluginManager::PLATFORM_OPFILE = "platform_ops.proto";

ProtoPluginManager plugins; // global manager, initializes on first use

ProtoPluginManager::ProtoPluginManager() {
  initialized = false;
}

void ProtoPluginManager::ensure_initialized(Args* args) {
  if(initialized>=2) return; // idempotent
  
  if(initialized==0) {
    if(!read_registry_file())
      cerr<<"WARNING: Only default plugins will be loaded.\n";
    lt_dlinit(); // begin using libltdl library tool
    initialized=1; // mark initialization complete
  }

  if(args!=NULL) {
    // Check command line to see if any other DLLs are being included locally
    while(args->extract_switch("--DLL",false)) { 
      string dll_name = args->pop_next();
      if(!read_dll(dll_name))
        { cerr << "Unable to include DLL " << dll_name << endl; }
    }
    initialized=2; // mark command line processing complete
  }
}

// helper function
void split(const string &s, const string &token, vector<string> &segments) {
  size_t i = 0;
  size_t j = 0;
  while (string::npos != (j = s.find(token, i))) {
    segments.push_back(s.substr(i, j - i));
    i = j + token.length();
  }
  segments.push_back(s.substr(i, s.length()));
}

bool ProtoPluginManager::parse_registry(istream &reg,string overridename) {
  while (!reg.eof()) {
    string line; getline(reg, line, '\n'); // get next line of registry
    // Ignore comment lines and empty lines.
    if (line.empty() || '#' == line.at(0)) continue;
    // If the line is precisely equal to the desired structure, slurp it
    vector<string> segments; split(line, " ", segments);
    if (segments.size() == 4 && segments[2] == "=") {
      string libname = (overridename=="")?segments[3]:overridename;
      registry[segments[0]][segments[1]] = libname;
    } else {
      cerr << "Unable to interpret registry line '" << line << "'" << endl;
    }
  }
}

bool ProtoPluginManager::read_dll(string libfile) {
  // open the library
  lt_dlhandle handle = lt_dlopenext(libfile.c_str());
  if(handle==NULL) 
    { cerr<<"Could not load plugin library "+libfile+"\n"; return false; }
  
  // parse the inventory
  void *fp = lt_dlsym(handle, "get_proto_plugin_inventory");
  if(fp==NULL)
    { cerr << libfile << "not a Proto plugin (no inventory)\n"; return false; }
  string reg((*((get_inventory_func)fp))());
  istringstream iss(reg);
  parse_registry(iss,libfile);
  
  // register the library
  fp = lt_dlsym(handle, "get_proto_plugin_library");
  if(fp==NULL) {
    cerr<<"Could not get get_proto_plugin_library from "+libfile; 
    return false;
  }
  ProtoPluginLibrary* lib = (*((get_library_func)fp))(); // run entry-point fn
  open_libs[libfile] = lib;
  
  return true;
}

bool ProtoPluginManager::read_registry_file() {
  string registryFile = PLUGIN_DIR+REGISTRY_FILE_NAME;
  ifstream fin; fin.open(registryFile.c_str());
  if(!fin.is_open()) 
    { cerr<<"Unable to open registry file: "<<registryFile<<endl; return false;}
  if(!parse_registry(fin)) return false;
  fin.close(); // Done: close the registry and return
  return true;
}

ProtoPluginLibrary* ProtoPluginManager::get_plugin_lib(string type,string name,Args* args){
  ensure_initialized(args);
  
  bool known = registry[type].count(name);
  if(!known)
    { cerr<<"No registered library contains "+type+" "+name+"\n"; return NULL; }
  string libfile = registry[type][name];

  ProtoPluginLibrary* lib;
  if(open_libs.count(libfile)) { lib = open_libs[libfile]; // use library...
  } else { // ... or load it in if it's not yet loaded
    string fullname = PLUGIN_DIR+libfile;
    lt_dlhandle handle = lt_dlopenext(fullname.c_str());
    if(handle==NULL) { 
      cerr<<"Could not load plugin library "+fullname+"\n"; 
      // Enable (and add right extension) if we need better debug info
      // dlopen(fullname.c_str(),RTLD_LAZY); cerr<<dlerror()<<endl;
      return NULL; 
    }
    void *fp = lt_dlsym(handle, "get_proto_plugin_library");
    if(fp==NULL) {
      cerr<<"Could not get get_proto_plugin_library from "+libfile; 
      return NULL;
    }
    lib = (*((get_library_func)fp))(); // run the entry-point function
    open_libs[libfile] = lib;
  }
  return lib;
}

void* ProtoPluginManager::get_sim_plugin(string type, string name, Args* args,
                                         SpatialComputer* cpu,int n){
  ProtoPluginLibrary* lib = get_plugin_lib(type,name,args);
  if(lib==NULL) return NULL;
  return lib->get_sim_plugin(type,name,args,cpu,n);
}

void* ProtoPluginManager::get_compiler_plugin(string type,string name,Args* args,Compiler* c){
  ProtoPluginLibrary* lib = get_plugin_lib(type,name,args);
  if(lib==NULL) return NULL;
  return lib->get_compiler_plugin(type,name,args,c);
}
