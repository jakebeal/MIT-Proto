/* 
 * File:   Torus.h
 * Author: prakash
 *
 * Created on February 5, 2010, 5:42 PM
 */

#ifndef _TORUS_H
#define	_TORUS_H

#include "Distribution.h"

class Torus : public Distribution {
public:
  METERS r, r_inner;
  Torus(Args* args, int n, Rect *volume);

  BOOL next_location(METERS *loc); 
};

#endif	/* _TORUS_H */

