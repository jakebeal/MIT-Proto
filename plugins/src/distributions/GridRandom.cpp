/* 
 * File:   GridRamdom.cpp
 * Author: prakash
 * 
 * Created on February 5, 2010, 5:41 PM
 */

#include "GridRandom.h"
#include "utils.h"

  GridRandom::GridRandom(Args* args, int n, Rect* volume) : Grid(n,volume) {
    epsilon = args->pop_number();
  }

  BOOL GridRandom::next_location(METERS *loc) {
    Grid::next_location(loc);
    loc[0] += epsilon*((rand()%1000/1000.0) - 0.5);
    loc[1] += epsilon*((rand()%1000/1000.0) - 0.5);
    if(volume->dimensions()==3) loc[2] += epsilon*((rand()%1000/1000.0) - 0.5);
    i++;
    return TRUE;
  }


