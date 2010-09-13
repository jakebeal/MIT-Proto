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

ProtoPluginManager plugins; // global manager, initializes on first use

ProtoPluginManager::ProtoPluginManager() {
  initialized = false;
}

void ProtoPluginManager::ensure_initialized() {
  if(initialized) return; // idempotent
  
  if(!read_registry()) cout<<"WARNING: Only default plugins will be loaded.\n";
  lt_dlinit(); // begin using libltdl library tool
  initialized = true;
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

bool ProtoPluginManager::read_registry() {
  string registryFile = PLUGIN_DIR+REGISTRY_FILE_NAME;
  ifstream fin; fin.open(registryFile.c_str());
  if(!fin.is_open()) 
    { cout<<"Unable to open registry file: "<<registryFile<<endl; return false;}
  
  while (!fin.eof()) {
    string line; getline(fin, line, '\n'); // get next line of registry
    // Ignore comment lines and empty lines.
    if (line.empty() || '#' == line.at(0)) continue;
    // If the line is precisely equal to the desired structure, slurp it
    vector<string> segments; split(line, " ", segments);
    if (segments.size() == 4 && segments[2] == "=") {
      registry[segments[0]][segments[1]] = segments[3];
    } else {
      cout << "Unable to interpret registry line '" << line << "'" << endl;
    }
  }
  fin.close(); // Done: close the registry and return
  return true;
}

void* ProtoPluginManager::get_sim_plugin(string type, string name, Args* args,
                                         SpatialComputer* cpu,int n){
  ensure_initialized();

  bool known = registry[type].count(name);
  if(!known)
    { cout<<"No registered library contains "+type+" "+name+"\n"; return NULL; }
  string libfile = registry[type][name];

  ProtoPluginLibrary* lib;
  if(open_libs.count(libfile)) { lib = open_libs[libfile]; // use library...
  } else { // ... or load it in if it's not yet loaded
    string fullname = PLUGIN_DIR+libfile;
    lt_dlhandle handle = lt_dlopenext(fullname.c_str());
    if(handle==NULL)
      { cout<<"Could not load plugin library "+fullname+"\n"; return NULL; }
    void *fp = lt_dlsym(handle, "get_proto_plugin_library");
    if(fp==NULL) {
      cout<<"Could not get get_proto_plugin_library from "+libfile; 
      return NULL;
    }
    lib = (*((get_library_func)fp))(); // run the entry-point function
    open_libs[libfile] = lib;
  }

  return lib->get_sim_plugin(type,name,args,cpu,n);
}

void* ProtoPluginManager::get_compiler_plugin(string type,string name,Args* args){
  ensure_initialized();

  uerror("Compiler plugins not yet implemented"); // TODO: implement compiler plugins
}
