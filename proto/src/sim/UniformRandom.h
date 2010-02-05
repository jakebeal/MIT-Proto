/* 
 * File:   UniformRandom.h
 * Author: prakash
 *
 * Created on February 5, 2010, 5:45 PM
 */

#ifndef _UNIFORMRANDOM_H
#define	_UNIFORMRANDOM_H

#include "Distribution.h"

class UniformRandom : public Distribution {
public:
  UniformRandom(int n, Rect* volume) ;
  virtual BOOL next_location(METERS *loc);
};

#endif	/* _UNIFORMRANDOM_H */

