/* 
 * File:   Grid.h
 * Author: prakash
 *
 * Created on February 5, 2010, 5:41 PM
 */

#ifndef _GRID_H
#define	_GRID_H

#include "Distribution.h"

class Grid : public Distribution {
public:
  int rows,columns,layers;
  int i;
  Grid(int n, Rect* volume) ;
  BOOL next_location(METERS *loc) ;
};

#endif	/* _GRID_H */

