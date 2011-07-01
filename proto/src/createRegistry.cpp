/* Registry creation app
Copyright (C) 2005-2010, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <sys/stat.h>

#include <dirent.h>
#include <dlfcn.h>
#include <ltdl.h>
#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "ir.h"
#include "plugin_manager.h"
#include "proto_plugin.h"
#include "spatialcomputer.h"

using namespace std;

static bool
dirExists(const string &name)
{
  struct stat st;
  if (stat(name.c_str(), &st) == 0) return true;
  cout << "Plugin directory " << name << " not found." << endl; // else...
  return false;
}

// Note: We might find that on Cygwin, libraries don't get the "lib" prefix.
static set<string>
getPotentialPlugins(const string &dir)
{
  set<string> candidates;
  DIR *dp; struct dirent *dirp;
  cout << "Searching for plugins...\n";

  // Get directory.
  if (!dirExists(dir))
    return candidates;

  if ((dp = opendir(dir.c_str())) == NULL)
    { cout << "Error opening " << dir << endl; return candidates; }

  // Scan all file names for potential plugins.
  while ((dirp = readdir(dp)) != NULL) {
    string name = string(dirp->d_name);
    // Possible plugin DLL if it starts with "lib".
    if (name.size() > 3 && name.substr(0, 3) == "lib") {
      size_t dot = name.rfind('.');
      if (dot > 0) name = name.substr(0, dot);
      // Record name minus extension.
      // FIXME: This doesn't strip the library version first...
      candidates.insert(name);
    }
  }

  closedir(dp);
  return candidates;
}

int
main(int argc, char **argv)
{
  // Begin using libltdl library tool.
  lt_dlinit();

  string dir = ProtoPluginManager::PLUGIN_DIR, allProperties = "";
  string registryFile = dir + ProtoPluginManager::REGISTRY_FILE_NAME;
  set<string> candidates = getPotentialPlugins(dir);
  if (candidates.size() == 0)
    cout << "Warning: no plugins found\n";

  for (set<string>::const_iterator i = candidates.begin();
       i != candidates.end();
       ++i) {
    cout << "Examining potential plugin " << (*i) << "... ";
    string dllFile = ProtoPluginManager::PLUGIN_DIR + (*i);
    lt_dlhandle handle = lt_dlopenext(dllFile.c_str());
    if (handle == NULL) {
      // Vague message because ltdl currently stomps error info.
      cout << "unable to open (unlinked symbol problem?)\n";
      cout << lt_dlerror() << endl;
      // FIXME: Hack for Mac OS X?  Shouldn't be necessary, ideally...
      void *h2 = dlopen((dllFile + ".dylib").c_str(), RTLD_NOW);
      cout << dlerror() << endl;
    } else {
      void *fp = lt_dlsym(handle, "get_proto_plugin_inventory");
      if (fp == NULL) {
        cout << "not a Proto plugin (no inventory)\n";
      } else {
        cout << "reading inventory\n";
        string tstring((*((get_inventory_func)fp))());
        // Show inventory in command-line output.
        print_indented(2, tstring, true);
        allProperties +=
          "# Inventory of plugin '" + (*i) + "'\n" + tstring + "\n";
      }
      // Done with this plugin.
      lt_dlclose(handle);
    }
  }

  // Close libltdl: no more libraries need to be read.
  lt_dlexit();

  cout << "Writing to registry\n";
  ofstream registry(registryFile.c_str());
  registry << allProperties;
  registry.close();
  if (registry.fail())
    cout << "Unable to write registry file.";

  return (EXIT_SUCCESS);
}

// Dummy declarations to fill in simulator names required to dlopen plugins.
Device *device = NULL;
SimulatedHardware *hardware = NULL;
MACHINE *machine = NULL;
void *palette = NULL;

// Touch a neocompiler element to ensure it gets linked in.
ProtoBoolean pb;
