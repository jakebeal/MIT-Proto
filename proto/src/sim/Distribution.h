/* 
 * File:   distribution.h
 * Author: prakash
 *
 * Created on February 5, 2010, 4:49 PM
 */

#ifndef _DISTRIBUTION_H
#define	_DISTRIBUTION_H

#include "utils.h"

// To create an initial distribution, we must know:
// - # nodes, volume they occupy, distribution type, any dist-specific args
class Distribution {
 public:
  int n; Rect *volume;
  METERS width, height, depth; // bounding box of volume occupied
  Distribution(int n, Rect *volume);
  virtual ~Distribution();
  // puts location in *loc and returns whether a device should be made
  virtual BOOL next_location(METERS *loc);// loc is a 3-vec
};

#endif	/* _DISTRIBUTION_H */

