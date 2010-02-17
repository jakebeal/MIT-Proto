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
    
    cout << "Trying to load dll: " << libFileName << endl;
    hand = dlopen(libFileName.c_str(), flag);
    //if (hand)
    //  break;
    //ext++;
  //}

  return hand;
}

/*
 * 
 */
int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        cout << "Usage " << argv[0] << " space_separated_lib_names " << endl;
        return (EXIT_SUCCESS);
    }
    int nLibs = argc - 1;
    vector<string> libNames;
    for(int i = 1; i < argc; i++)
    {
        libNames.push_back(argv[i]);
    }
    for(int i = 0; i < nLibs; i++)
    {
        cout << "loading dll: " << libNames[i] << endl;
        void *handle = dlopenext(libNames[i].c_str());
        if(handle == NULL)
        {
            cout << "DLL not found: " << libNames[i] << endl;
        }
        else
        {
            cout << "Success loading: " << libNames[i] << endl;
        }

        if(handle != NULL)
        {
            dlclose(handle);
        }
        cout << endl;
    }
    return (EXIT_SUCCESS);
}

