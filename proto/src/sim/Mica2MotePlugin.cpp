/* 
 * File:   Mica2MotePlugin.cpp
 * Author: prakash
 * 
 * Created on February 18, 2010, 5:35 PM
 */

#include "Mica2MotePlugin.h"
#include "Mica2Mote.h"
#include <sstream>
using namespace std;

Mica2MotePlugin::Mica2MotePlugin()
{
    layerName = MICA2MOTE_NAME;
}
Layer* Mica2MotePlugin::get_layer(char* name, Args* args,SpatialComputer* cpu, int n)
{
    if(layerName == string(name))
    {
        new MoteIO(args,cpu);
    }
    return NULL;
}

string Mica2MotePlugin::getProperties()
{
    stringstream ss;
    ss << "Layer " << MICA2MOTE_NAME << " = " << MICA2MOTE_DLL_NAME << endl;
    return ss.str();
}

#ifdef __cplusplus

extern "C" {

ProtoPluginLibrary* get_proto_plugins()
{
    return new Mica2MotePlugin();
}
const char* get_proto_plugin_properties()
{
    string propS = Mica2MotePlugin::getProperties();
    char *props = new char[propS.size() + 1];
    strncpy(props, propS.c_str(), propS.size());
    props[propS.size()] = '\0';
    return props;
}

}

#endif
