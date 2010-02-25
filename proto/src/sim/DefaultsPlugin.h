/*
 * DefaultsPlugin.h
 *
 *  Created on: Feb 24, 2010
 *      Author: gbays
 */
#include <string>
#include "ProtoPluginLibrary.h"
#include "spatialcomputer.h"
#include "sim-hardware.h"
#include "basic-hardware.h"
#include "simpledynamics.h"
#include "unitdiscradio.h"
#include "Distribution.h"
#include "UniformRandom.h"
#include "FixedIntervalTime.h"

#ifndef DEFAULTSPLUGIN_H_
#define DEFAULTSPLUGIN_H_

class DefaultsPlugin : public ProtoPluginLibrary {
public:
  DefaultsPlugin();
  virtual ~DefaultsPlugin();

  Layer* get_layer(char* name, Args* args,SpatialComputer* cpu, int n);

  Distribution* get_distribution(char* name, Args* args,SpatialComputer* cpu, int n);

  TimeModel* get_time_model(char* name, Args* args,SpatialComputer* cpu, int n);

private:
  Rect* mDistVolume;
};

#endif /* DEFAULTSPLUGIN_H_ */
