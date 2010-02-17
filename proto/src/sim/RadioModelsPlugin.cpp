/* 
 * File:   RadioModels.cpp
 * Author: prakash
 * 
 * Created on February 8, 2010, 1:41 PM
 */

#include "RadioModelsPlugin.h"
#include "multiradio.h"
#include "wormhole-radio.h"

#include <sstream>
using namespace std;

RadioModelsPlugin::RadioModelsPlugin()
{
    wormholesName = WORM_HOLES_NAME;
    multiradioName = MULTI_RADIO_NAME;
}
Layer* RadioModelsPlugin::get_layer(char* name, Args* args,SpatialComputer* cpu, int n)
{
    if(string(name) == wormholesName)
    {
        return new WormHoleRadio(args,cpu,n);
    }
    if(string(name) == multiradioName)
    {
        new MultiRadio(args, cpu, n);
    }
    return NULL;
}

string RadioModelsPlugin::getProperties()
{
    stringstream ss;
    ss << "Layer " << WORM_HOLES_NAME << " = " << RADIO_MODELS_DLL_NAME << endl;
    ss << "Layer " << MULTI_RADIO_NAME << " = " << RADIO_MODELS_DLL_NAME << endl;
    return ss.str();
}

#ifdef __cplusplus

extern "C" {

ProtoPluginLibrary* get_proto_plugins()
{
    return new RadioModelsPlugin();
}
const char* get_proto_plugin_properties()
{
    string propS = RadioModelsPlugin::getProperties();
    char *props = new char[propS.size() + 1];
    props[propS.size()] = '\0';
    return props;
}

}
#endif

