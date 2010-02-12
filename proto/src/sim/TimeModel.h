/*
 * TimeModel.h
 *
 *  Created on: Feb 12, 2010
 *      Author: gbays
 */
#include "utils.h"
#include "DeviceTimer.h"

#ifndef TIMEMODEL_H_
#define TIMEMODEL_H_

class TimeModel {
 public:
  virtual DeviceTimer* next_timer(SECONDS* start_lag)=0;
  virtual SECONDS cycle_time()=0;
};

#endif /* TIMEMODEL_H_ */
