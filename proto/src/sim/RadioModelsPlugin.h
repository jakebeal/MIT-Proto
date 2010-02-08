/* 
 * File:   RadioModels.h
 * Author: prakash
 *
 * Created on February 8, 2010, 1:41 PM
 */

#ifndef _RADIOMODELSPLUGIN_H
#define	_RADIOMODELS_H

#include "ProtoPluginLibrary.h"

// Plugin class
class RadioModelsPlugin : public ProtoPluginLibrary {
private:
    string wormholesName;
    string multiradioName;
public:
    RadioModelsPlugin();
    Layer* get_layer(char* name, Args* args,SpatialComputer* cpu, int n);

};

#endif	/* _RADIOMODELSPLUGIN_H */

