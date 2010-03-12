/* 
 * File:   Torus.cpp
 * Author: prakash
 * 
 * Created on February 5, 2010, 5:42 PM
 */

#include "Torus.h"

const flo TWO_PI = 2 * M_PI;

  Torus::Torus(Args* args, int n, Rect *volume) : Distribution(n, volume) {
    METERS outer = MIN(width, height) / 2;
    flo ratio = (str_is_number(args->peek_next()) ? args->pop_number() : 0.75);
    r = ratio * outer;
    r_inner = outer - r;
  }

  BOOL Torus::next_location(METERS *loc) {
    flo theta = urnd(0, TWO_PI);
    if(volume->dimensions() == 3) {
      flo phi = urnd(0, TWO_PI);
      flo rad = urnd(0, r_inner);
      loc[0] = (r + rad * cos(phi)) * cos(theta);
      loc[1] = (r + rad * cos(phi)) * sin(theta);
      loc[2] = rad * sin(phi);
    } else {
      flo rad = r + urnd(-r_inner, r_inner);
      loc[0] = rad * cos(theta);
      loc[1] = rad * sin(theta);
    }
  }

