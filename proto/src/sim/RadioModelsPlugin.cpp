/* 
 * File:   RadioModels.cpp
 * Author: prakash
 * 
 * Created on February 8, 2010, 1:41 PM
 */

#include "RadioModelsPlugin.h"
#include "multiradio.h"
#include "wormhole-radio.h"

RadioModelsPlugin::RadioModelsPlugin()
{
    wormholesName = "wormholes";
    multiradioName = "multiradio";
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

