/* Registry creation app
Copyright (C) 2005-2010, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <fstream>
#include <dlfcn.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ltdl.h>
#include "proto_plugin.h"
#include "plugin_manager.h"
#include "spatialcomputer.h"
using namespace std;

bool dirExists(string name) {
  struct stat st;
  if(stat(name.c_str(),&st) == 0) return true;
  cout << "Plugin directory " << name << " not found." << endl; // else...
  return false;
}

// Note: we might find that on Cygwin, libraries don't get the "lib" prefix
set<string> getPotentialPlugins(string dir) {
  set<string> candidates;
  DIR *dp; struct dirent *dirp;
  cout << "Searching for plugins...\n";
  // get directory
  if(!dirExists(dir)) { return candidates; }
  if((dp = opendir(dir.c_str())) == NULL) 
    { cout << "Error opening " << dir << endl; return candidates; }
  // scan all file names for potential plugins
  while ((dirp = readdir(dp)) != NULL) {
    string name = string(dirp->d_name);
    // possible plugin DLL if it starts with "lib"
    if(name.size()>3 && name.substr(0,3)=="lib") {
      size_t dot=name.rfind('.'); if(dot>0) name=name.substr(0,dot);
      candidates.insert(name); // record name minus extension
    }
  }
  // close & return
  closedir(dp);
  return candidates;
}

int main(int argc, char** argv) {
  lt_dlinit(); // begin using libltdl library tool
  string dir = ProtoPluginManager::PLUGIN_DIR, allProperties = "";
  string registryFile = dir+ProtoPluginManager::REGISTRY_FILE_NAME;
  
  set<string> candidates = getPotentialPlugins(dir);
  if(candidates.size()==0) cout << "Warning: no plugins found";
  for(set<string>::iterator i = candidates.begin(); i!=candidates.end();i++) {
    cout << "Examining potential plugin " << *i << "... ";
    string dllFile = ProtoPluginManager::PLUGIN_DIR+(*i);
    lt_dlhandle handle = lt_dlopenext(dllFile.c_str());
    if(handle == NULL) { // Vague msg because ltdl currently stomps error info
      cout << "unable to open (unlinked symbol problem?)\n";
      cout << lt_dlerror() << endl;
      void* h2 = dlopen((dllFile+".dylib").c_str(),RTLD_NOW);
      cout << dlerror() << endl;
    } else {
      void *fp = lt_dlsym(handle, "get_proto_plugin_inventory");
      if(fp==NULL) { cout << "not a Proto plugin (no inventory)\n";
      } else {
        cout << "reading inventory\n";
        string tstring((*((get_props_func)fp))());
        print_indented(2,tstring,true); // show inventory in cmdline output
        allProperties += "# Inventory of plugin '"+(*i)+"'\n"+tstring+"\n";
      }
      lt_dlclose(handle); // done with this plugin
    }
  }
  lt_dlexit(); // close libltdl: no more libraries need to be read
  
  cout << "Writing to registry\n";
  ofstream registry; registry.open(registryFile.c_str());
  registry << allProperties; registry.close(); 
  if(registry.fail()) cout << "Unable to write registry file.";
  return (EXIT_SUCCESS);
}

// Dummy declarations to fill in simulator names required to dlopen plugins
Device* device = NULL;
SimulatedHardware* hardware = NULL;
MACHINE* machine = NULL;
void* palette = NULL;
