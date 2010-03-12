/* 
 * File:   Mica2MotePlugin.h
 * Author: prakash
 *
 * Created on February 18, 2010, 5:35 PM
 */

#ifndef _MICA2MOTEPLUGIN_H
#define	_MICA2MOTEPLUGIN_H

#include "ProtoPluginLibrary.h"

#define MICA2MOTE_NAME "mote-io"
#define MICA2MOTE_DLL_NAME "mica2mote"

// Plugin class
class Mica2MotePlugin : public ProtoPluginLibrary {
private:
    string layerName;
public:
    Mica2MotePlugin();
    Layer* get_layer(char* name, Args* args,SpatialComputer* cpu, int n);
    static string getProperties();
};

#endif	/* _MICA2MOTEPLUGIN_H */

