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
#include "dlfcn.h"
using namespace std;

void* dlopenext(const char *name, int flag = RTLD_NOW)
{
  //const char **ext = dl_exts;
  void *hand = NULL;
  string prefix = "unkown_lib_prefix";
  string ext = "unknown_lib_extension";

#ifdef __CYGWIN__ 
    prefix = "";
    ext = ".dll";
  #else // assume other platforms to be Unix/Linux
    prefix = "lib";
    #ifdef __APPLE__
        ext = ".dylib" ;
    #else // assume all other platforms have .so extension
        ext = ".so";
    #endif
#endif

    stringstream ss;
    ss << prefix << string(name) << ext;
    string libFileName = ss.str();
    
    //cout << "Trying to load dll: " << libFileName << endl;
    hand = dlopen(libFileName.c_str(), flag); 

  return hand;
}

typedef const char* (*get_props_func)(void);

/*
 * 
 */
int main(int argc, char** argv)
{
    if(argc < 3)
    {
        cout << "Usage " << argv[0] << " registry_file space_separated_lib_names " << endl;
        return (EXIT_SUCCESS);
    }
    int nLibs = argc - 2;
    vector<string> libNames;
    string registryFile = argv[1];
    
    for(int i = 2; i < argc; i++)
    {
        libNames.push_back(argv[i]);
    }
    get_props_func props_function_instance = NULL;
    vector<string> allProperties;
    vector<void *> handles;
    for(int i = 0; i < nLibs; i++)
    {
        //cout << "loading dll: " << libNames[i] << endl;
        void *handle = dlopenext(libNames[i].c_str());
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

