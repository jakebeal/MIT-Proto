/* 
 * File:   UniformRandom.cpp
 * Author: prakash
 * 
 * Created on February 5, 2010, 5:45 PM
 */

#include "UniformRandom.h"


  UniformRandom::UniformRandom(int n, Rect* volume) : Distribution(n,volume) {}
  BOOL UniformRandom::next_location(METERS *loc) {
    loc[0] = urnd(volume->l,volume->r);
    loc[1] = urnd(volume->b,volume->t);
    if(volume->dimensions()==3) {
      Rect3* r = (Rect3*)volume;
      loc[2] = urnd(r->f,r->c);
    } else loc[2]=0;
    return TRUE;
  }

