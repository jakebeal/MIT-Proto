/* 
 * File:   createRegistry.cpp
 * Author: prakash
 *
 * Created on February 17, 2010, 2:59 PM
 */

#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <fstream>
#include <dlfcn.h>
#include <sys/stat.h>
#include <algorithm>
#include <dirent.h>
#include <ltdl.h>
#include "config.h"
#include "utils.h"
#include "proto.h"
#include "spatialcomputer.h"
using namespace std;

#define REGISTRY_FILE_NAME "registry.txt"

void print_indented(int n, string s) {
  int loc = 0;
  while(true) {
    int newloc = s.find('\n',loc);
    for(int i=0;i<n;i++) cout << " ";
    cout << s.substr(loc,newloc-loc) << endl;
    loc=newloc+1; if(newloc<0) return;
  }
}

bool dirExists(string name) {
  struct stat st;
  bool exists = false;
  
  if(stat(name.c_str(),&st) == 0) { exists = true;        
  } else { cout << "dir: " << name << " does not exist." << endl;
  }
  return exists;
}

set<string> getInstalledPlugins(string dir) {
  set<string> pluginDlls;
  DIR *dp; struct dirent *dirp;
  // get directory
  if((dp = opendir(dir.c_str())) == NULL) {
    cout << "Error opening " << dir << endl;
    return pluginDlls;
  }
  // snag all file names as potential plugins
  while ((dirp = readdir(dp)) != NULL) {
    string name = string(dirp->d_name);
    // possible plugin DLL if it starts with "lib"
    if(name.size()>3 && name.substr(0,3)=="lib") {
      size_t dot = name.rfind('.');
      if(dot>0) name = name.substr(0,dot); // strip extension
      pluginDlls.insert(name);
    }
  }
  // close & return
  closedir(dp);
  return pluginDlls;
}

vector<string> getPluginNames()
{
    // assume our working dir is DllUtils::PLUGIN_DIR
    vector<string> pluginNames;
    if(!dirExists(DllUtils::PLUGIN_DIR)) { return pluginNames; }
    
    cout << "Searching for plugins...\n";
    set<string> pluginFiles = getInstalledPlugins(DllUtils::PLUGIN_DIR);
    set<string>::iterator i = pluginFiles.begin();
    for(; i!=pluginFiles.end();i++) {
      string dllName = *i;
      //cout << "found plugin: " << dllName << endl;
      pluginNames.push_back(dllName);
    }

    return pluginNames;
}

typedef const char* (*get_props_func)(void);

/*
 * 
 */
int main(int argc, char** argv) {
  //  MACHINE* m = machine; cout << "Machine = " << (size_t)(m) << endl;
  lt_dlinit(); // begin using libltdl
  
  vector<string> libNames = getPluginNames();
  
  string registryFile = string(DllUtils::PLUGIN_DIR).append(REGISTRY_FILE_NAME);
  
  get_props_func props_function_instance = NULL;
  vector<string> allProperties;
  for(int i = 0; i < libNames.size(); i++) {
    cout << "Examining potential plugin " << libNames[i] << "... ";
    string dllFile = string(DllUtils::PLUGIN_DIR).append(libNames[i]);
    //cout << "Loading dll file name: " << dllFile << endl;
    lt_dlhandle handle = lt_dlopenext(dllFile.c_str());
    //void* handle = dlopen(dllFile.c_str(), RTLD_NOW);
    if(handle == NULL) { // Vague msg because ltdl currently stomps error info
      cout << "unable to open (maybe an unlinked symbol problem).\n";
    } else {
      //void *fp = dlsym(handle, "get_proto_plugin_properties");
      void *fp = lt_dlsym(handle, "get_proto_plugin_properties");
      if(fp==NULL) {
        cout << "no Proto plugin properties.\n";
      } else {
        cout << "reading properties.\n";
        props_function_instance = (get_props_func)fp;
        const char *props = (*props_function_instance)();
        //            string props((*props_function_instance)());
        string tstring(props);
        print_indented(2,tstring);
        allProperties.push_back(tstring);
      }
      lt_dlclose(handle);
    }
  }

  cout << "Writing to registry";
  ofstream registry; registry.open(registryFile.c_str());
  for(int i = 0; i < allProperties.size(); i++) {
    cout << ".";
    registry << "# Inventory of plugin '"<<libNames[i]<<"'"<<endl;
    registry << allProperties[i] << endl;
    //cout << allProperties[i] << endl;
  }
  registry.close();
  cout << endl;
  lt_dlexit(); // close libltdl
  return (EXIT_SUCCESS);
}

// To fill in gaps in the linking:
Device* device = NULL;
SimulatedHardware* hardware = NULL;
MACHINE* machine = NULL;
