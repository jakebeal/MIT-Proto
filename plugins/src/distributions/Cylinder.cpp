/* 
 * File:   Cylinder.cpp
 * Author: prakash
 * 
 * Created on February 5, 2010, 5:42 PM
 */

#include "Cylinder.h"


  Cylinder::Cylinder(int n, Rect* volume) : Distribution(n,volume) {
    r = height / 2;
  }
  BOOL Cylinder::next_location(METERS *loc) {
    loc[0] = urnd(volume->l,volume->r);
    flo theta = urnd(0, 2 * 3.14159);
    loc[1] = r * sin(theta);
    loc[2] = r * cos(theta);
    return TRUE;
  }

