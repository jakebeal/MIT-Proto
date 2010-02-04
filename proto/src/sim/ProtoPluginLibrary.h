/*
 * ProtoPluginLibrary.h
 *
 *  Created on: Jan 28, 2010
 *      Author: gbays
 */

#include "spatialcomputer.h"

#ifndef PROTOPLUGINLIBRARY_H_
#define PROTOPLUGINLIBRARY_H_

class Layer;
class TimeModel;
class Distribution;
class SpatialComputer;


class ProtoPluginLibrary
{
public:

	virtual Layer* get_layer(char* name, Args* args,SpatialComputer* cpu, int n)
	{ return NULL; }

	virtual TimeModel* get_time_model(char* name, Args* args,SpatialComputer* cpu, int n)
	{ return NULL; }

	virtual Distribution* get_distribution(char* name, Args* args,SpatialComputer* cpu, int n)
	{ return NULL; }

};

#endif /* PROTOPLUGINLIBRARY_H_ */


#ifdef __cplusplus

extern "C" {

ProtoPluginLibrary* get_proto_plugins();
const char* get_proto_plugin_properties();

}
#endif


