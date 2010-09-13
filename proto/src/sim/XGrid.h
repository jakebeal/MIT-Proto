/* 
 * File:   XGrid.h
 * Author: prakash
 *
 * Created on February 5, 2010, 5:32 PM
 */

#ifndef _XGRID_H
#define	_XGRID_H

#include "proto_plugin.h"
#include "spatialcomputer.h"

// Y is random
class XGrid : public Distribution {
public:
  int rows,columns,layers;
  int i;
  XGrid(int n, Rect* volume);
  BOOL next_location(METERS *loc);
};

#endif	/* _XGRID_H */

