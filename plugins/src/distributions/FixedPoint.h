/* 
 * File:   FixedPoint.h
 * Author: prakash
 *
 * Created on February 5, 2010, 5:40 PM
 */

#ifndef _FIXEDPOINT_H
#define	_FIXEDPOINT_H

#include "UniformRandom.h"

class FixedPoint : public UniformRandom {
public:
  int fixed; int n_fixes;
  Population fixes;
  FixedPoint(Args* args, int n, Rect* volume);
  virtual ~FixedPoint();
  BOOL next_location(METERS *loc); 
};

#endif	/* _FIXEDPOINT_H */

