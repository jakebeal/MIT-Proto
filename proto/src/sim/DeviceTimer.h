/*
 * DeviceTimer.h
 *
 *  Created on: Feb 12, 2010
 *      Author: gbays
 */
#include "utils.h"

#ifndef DEVICETIMER_H_
#define DEVICETIMER_H_

class DeviceTimer {
 public:  // both of these report delay from the current compute time
  virtual void next_transmit(SECONDS* d_true, SECONDS* d_internal)=0;
  virtual void next_compute(SECONDS* d_true, SECONDS* d_internal)=0;
  virtual DeviceTimer* clone_device()=0; // split the timer for a clone dev
};

#endif /* DEVICETIMER_H_ */
