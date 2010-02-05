/* 
 * File:   FixedPoint.cpp
 * Author: prakash
 * 
 * Created on February 5, 2010, 5:40 PM
 */

#include "FixedPoint.h"

  FixedPoint::FixedPoint(Args* args, int n, Rect* volume) : UniformRandom(n,volume) {
    fixed=0; n_fixes=0;
    do {
      n_fixes++;
      METERS* loc = (METERS*)calloc(3,sizeof(METERS));
      loc[0]=args->pop_number(); loc[1]=args->pop_number();
      loc[2]= (str_is_number(args->peek_next()) ? args->pop_number() : 0);
      fixes.add(loc);
    } while(args->extract_switch("-fixedpt",FALSE));
  }
  FixedPoint::~FixedPoint() {
    for(int i=0;i<n_fixes;i++) { free(fixes.get(i)); }
  }
  BOOL FixedPoint::next_location(METERS *loc) {
    if(fixed<n_fixes) {
      METERS* src = (METERS*)fixes.get(fixed);
      for(int i=0;i<3;i++) loc[i]=src[i];
      fixed++;
      return TRUE;
    } else {
      return UniformRandom::next_location(loc);
    }
  }

