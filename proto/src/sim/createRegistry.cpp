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
#include <sstream>
#include <fstream>
#include <dlfcn.h>
#include "utils.h"
#include <sys/stat.h>
#include <algorithm>
#include <dirent.h>
using namespace std;

#define REGISTRY_FILE_NAME "plugins_registry.txt"

bool dirExists(string name)
{
    struct stat st;
    bool exists = false;
    
    if(stat(name.c_str(),&st) == 0)
    {
        exists = true;        
    }
    else
    {
        cout << "dir: " << name << " does not exists." << endl;
    }
    return exists;
}

vector<string> getInstalledPlugins(string dir)
{
    vector<string> pluginDlls;

    DIR *dp;
    struct dirent *dirp;

    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error("  << ") opening " << dir << endl;
        return pluginDlls;
    }

    while ((dirp = readdir(dp)) != NULL) {
        string name = string(dirp->d_name);
        if(name != string(".") && name != string("..") )
            pluginDlls.push_back(name);
    }

    closedir(dp);
    

    return pluginDlls;
}

vector<string> getPluginNames()
{
    // assume out working dir has INSTALLED_PLUGINS_DIR
    vector<string> pluginNames;
    if(!dirExists(INSTALLED_PLUGINS_DIR))
    {
        return pluginNames;
    }

    vector<string> pluginFiles = getInstalledPlugins(INSTALLED_PLUGINS_DIR);
    string platformPrefix;
    string platformExt;
    DllUtils::getPlatformLib(platformPrefix, platformExt);
    for(int i = 0; i < pluginFiles.size(); i++)
    {
        string dir_ent = pluginFiles[i];
        string dllName = dir_ent;
        // Strip prefix from libName.so
        if(platformPrefix.size() > 0)
        {
            dllName.erase(0,platformPrefix.size());
            //cout << "Stripped prefix: " << dllName << endl;
        }

        size_t where = dllName.find(platformExt);
        //cout << "found ext at: " << where << endl;
        dllName.erase(where);
        cout << "found plugin: " << dllName << endl;
        pluginNames.push_back(dllName);
    }
    return pluginNames;
}

typedef const char* (*get_props_func)(void);

/*
 * 
 */
int main(int argc, char** argv)
{
   vector<string> libNames = getPluginNames();
            
   string registryFile = REGISTRY_FILE_NAME;
       
    get_props_func props_function_instance = NULL;
    vector<string> allProperties;
    vector<void *> handles;
    string prefix;
    string ext;
    DllUtils::getPlatformLib(prefix,ext);
    for(int i = 0; i < libNames.size(); i++)
    {
        cout << "loading dll: " << libNames[i] << endl;
        string dllFile = string(INSTALLED_PLUGINS_DIR).append("/").append(prefix).append(libNames[i]).append(ext);
        cout << "Loading dll file name: " << dllFile << endl;
        void *handle = dlopen(dllFile.c_str(), RTLD_NOW);
        if(handle == NULL)
        {
            cout << "DLL not found: " << libNames[i] << endl;
        }
        else
        {
            handles.push_back(handle);
            //cout << "Success loading: " << libNames[i] << endl;
            //demo_function = (void (*)(void))dlsym(handle, "get_proto_plugin_properties");
            void *fp = dlsym(handle, "get_proto_plugin_properties");
            if(fp != NULL)
            {
            props_function_instance = (get_props_func)fp;
            const char *props = (*props_function_instance)();
//            string props((*props_function_instance)());
            string tstring(props);
            //cout << string(tstring) ;//<< endl;
            allProperties.push_back(tstring);
            }
            else
            {
                cout << "Error retrieving function name:\n" << dlerror() << endl;
            }
        }

        
        //cout << endl << "finished with a dll \n"<< endl;
    }

    for(int i = 0; i < handles.size(); i++)
    {
        void *handle = handles[i];
        
        dlclose(handle);
    }

  ofstream registry;
  registry.open (registryFile.c_str());
  


    for(int i = 0; i < allProperties.size(); i++)
    {
        registry << "# Properties for shared object / DLL '" << libNames[i] << "'" << endl;
        registry << allProperties[i] << endl;;
        cout << allProperties[i] << endl;
    }
    registry.close();
    return (EXIT_SUCCESS);
}

