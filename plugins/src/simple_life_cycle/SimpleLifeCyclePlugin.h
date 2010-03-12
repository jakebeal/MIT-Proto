/* 
 * File:   SimpleLifeCyclePlugin.h
 * Author: prakash
 *
 * Created on February 18, 2010, 5:25 PM
 */

#ifndef _SIMPLELIFECYCLEPLUGIN_H
#define	_SIMPLELIFECYCLEPLUGIN_H

#include "ProtoPluginLibrary.h"

#define SIMPLE_LIFE_CYCLE_NAME "simple-life-cycle"
#define SIMPLE_LIFE_CYCLE_DLL_NAME SIMPLE_LIFE_CYCLE_NAME

// Plugin class
class SimpleLifeCyclePlugin : public ProtoPluginLibrary {
private:
    string layerName;
public:
    SimpleLifeCyclePlugin();
    Layer* get_layer(char* name, Args* args,SpatialComputer* cpu, int n);

    static string getProperties();
};

#endif	/* _SIMPLELIFECYCLEPLUGIN_H */

