/* 
 * File:   Cylinder.h
 * Author: prakash
 *
 * Created on February 5, 2010, 5:42 PM
 */

#ifndef _CYLINDER_H
#define	_CYLINDER_H

#include "Distribution.h"

class Cylinder : public Distribution {
public:
  METERS r;
  Cylinder(int n, Rect* volume);
  BOOL next_location(METERS *loc);
};

#endif	/* _CYLINDER_H */

