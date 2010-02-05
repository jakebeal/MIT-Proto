/* 
 * File:   Grid.cpp
 * Author: prakash
 * 
 * Created on February 5, 2010, 5:41 PM
 */

#include "Grid.h"

  Grid::Grid(int n, Rect* volume) : Distribution(n,volume) {
    i=0;
    if(volume->dimensions()==3) {
      layers = (int)ceil(pow(n*depth*depth/(width*height), 1.0/3.0));
      rows = (int)ceil(layers*width/depth);
      columns = (int)ceil(n/rows/layers);
    } else {
      rows = (int)ceil(sqrt(n)*sqrt(width/height));
      columns = (int)ceil(n/rows);
      layers = 1;
    }
  }
  BOOL Grid::next_location(METERS *loc) {
    int l = (i%layers), r = (i/layers)%rows, c = (i/(layers*rows));
    loc[0] = volume->l + c*width/columns;
    loc[1] = volume->b + r*height/rows;
    loc[2] = (volume->dimensions()==3)?(((Rect3*)volume)->f+l*depth/layers):0;
    i++;
    return TRUE;
  }


