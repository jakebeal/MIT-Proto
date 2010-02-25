/*
 * FixedIntervalTime2.h
 *
 *  Created on: Feb 24, 2010
 *      Author: gbays
 */

#include "TimeModel.h"
#include "sim-hardware.h"
#include "spatialcomputer.h"

#ifndef FIXEDINTERVALTIME_H_
#define FIXEDINTERVALTIME_H_

class SpatialComputer;

class FixedTimer : public DeviceTimer {
  SECONDS dt, half_dt, internal_dt, internal_half_dt;
  flo ratio;
public:
  FixedTimer(flo dt, flo ratio);

  void next_transmit(SECONDS* d_true, SECONDS* d_internal);

  void next_compute(SECONDS* d_true, SECONDS* d_internal);

  DeviceTimer* clone_device() { return new FixedTimer(dt,internal_dt/dt); }
  void set_internal_dt(SECONDS dt);

};

class FixedIntervalTime : public TimeModel, public HardwarePatch {
  BOOL sync;
  flo dt; flo var;
  flo ratio; flo rvar;  // ratio is internal/true time
public:
  FixedIntervalTime(Args* args, SpatialComputer* p);
  virtual ~FixedIntervalTime();

  DeviceTimer* next_timer(SECONDS* start_lag);

  SECONDS cycle_time() { return dt; }
  NUM_VAL set_dt (NUM_VAL dt);

};

#endif /* FIXEDINTERVALTIME_H_ */
