/* 
 * File:   GridRamdom.h
 * Author: prakash
 *
 * Created on February 5, 2010, 5:41 PM
 */

#ifndef _GRIDRANDOM_H
#define	_GRIDRANDOM_H

#include "Grid.h"

class GridRandom : public Grid {
public:
  METERS epsilon;
  GridRandom(Args* args, int n, Rect* volume);
  BOOL next_location(METERS *loc);
};

#endif	/* _GRIDRANDOM_H */

