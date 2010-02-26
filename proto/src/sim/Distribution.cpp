/* 
 * File:   distribution.cpp
 * Author: prakash
 * 
 * Created on February 5, 2010, 4:49 PM
 */

#include "Distribution.h"

// To create an initial distribution, we must know:
// - # nodes, volume they occupy, distribution type, any dist-specific args

  Distribution::Distribution(int n, Rect *volume) { // subclasses often take an Args* too
    this->n=n; this->volume=volume;
    width = volume->r-volume->l; height = volume->t-volume->b; depth=0;
    if(volume->dimensions()==3) depth=((Rect3*)volume)->c-((Rect3*)volume)->f;
  }

  Distribution::~Distribution() {
    delete volume;
    volume = NULL;
  }

  // puts location in *loc and returns whether a device should be made
  BOOL Distribution::next_location(METERS *loc) { return FALSE; } // loc is a 3-vec


