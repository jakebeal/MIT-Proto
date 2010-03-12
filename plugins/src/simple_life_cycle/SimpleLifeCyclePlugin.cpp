/* 
 * File:   SimpleLifeCyclePlugin.cpp
 * Author: prakash
 * 
 * Created on February 18, 2010, 5:25 PM
 */

#include "SimpleLifeCyclePlugin.h"
#include "SimpleLifeCycle.h"
#include <sstream>
using namespace std;

SimpleLifeCyclePlugin::SimpleLifeCyclePlugin()
{
    layerName = SIMPLE_LIFE_CYCLE_NAME;
}

Layer* SimpleLifeCyclePlugin::get_layer(char* name, Args* args,SpatialComputer* cpu, int n)
{
    if(string(name) == layerName)
    {
        return new SimpleLifeCycle(args, cpu);
    }
    return NULL;
}

string SimpleLifeCyclePlugin::getProperties()
{
    stringstream ss;
    ss << "Layer " << SIMPLE_LIFE_CYCLE_NAME << " = " << SIMPLE_LIFE_CYCLE_DLL_NAME << endl;
    return ss.str();
}

#ifdef __cplusplus

extern "C" {

ProtoPluginLibrary* get_proto_plugins()
{
    return new SimpleLifeCyclePlugin();
}
const char* get_proto_plugin_properties()
{
    string propS = SimpleLifeCyclePlugin::getProperties();
    char *props = new char[propS.size() + 1];
    strncpy(props, propS.c_str(), propS.size());
    props[propS.size()] = '\0';
    return props;
}

}
#endif

