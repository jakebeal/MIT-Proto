/*
 * DefaultsPlugin.cpp
 *
 *  Created on: Feb 24, 2010
 *      Author: gbays
 */

#include "DefaultsPlugin.h"

DefaultsPlugin::DefaultsPlugin():mDistVolume(NULL) {


}

DefaultsPlugin::~DefaultsPlugin() {

  delete mDistVolume; // since we cloned the Rect ptr, we had better clean it up here.
}

Layer* DefaultsPlugin::get_layer(char* name, Args* args,SpatialComputer* cpu, int n)
{
   string nameStr = name;
   Layer* layerPtr = NULL;

   if (nameStr.compare("DebugLayer") == 0){
     layerPtr = new DebugLayer(args, cpu);
   }
   else if (nameStr.compare("PerfectLocalizer") == 0){
     layerPtr = new PerfectLocalizer(cpu);
   }
   else if (nameStr.compare("UnitDiscRadio") == 0){
     layerPtr = new UnitDiscRadio(args, cpu, n);
   }
   else if (nameStr.compare("SimpleDynamics") == 0){
     layerPtr = new SimpleDynamics(args, cpu, n);
   }

   return layerPtr;

}

Distribution* DefaultsPlugin::get_distribution(char* name, Args* args,SpatialComputer* cpu, int n)
{
  mDistVolume = cpu->volume->clone();
  if(args->extract_switch("-dist-dim")) {
    mDistVolume->l = args->pop_number();
    mDistVolume->r = args->pop_number();
    mDistVolume->b = args->pop_number();
    mDistVolume->t = args->pop_number();
    if(mDistVolume->dimensions()==3) {
      ((Rect3*)mDistVolume)->f = args->pop_number();
      ((Rect3*)mDistVolume)->c = args->pop_number();
    }
  }

  Distribution* distPtr = new UniformRandom(n, mDistVolume);

  return distPtr;

}

TimeModel* DefaultsPlugin::get_time_model(char* name, Args* args,SpatialComputer* cpu, int n)
{
   TimeModel* timeModelPtr = new FixedIntervalTime(args, cpu);

   return timeModelPtr;

}

