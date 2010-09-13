#include "spatialcomputer.h"

//note: because the following classes inherit from DeviceTimer,TimeModel, Distribution
// classes that are defined in this file they have to be here rather than in their own file.
// Forward declarations of them will not work for inheritance in other files.
// Classes DeviceTimer, TimeModel, Distribution, and Layer should all be moved to their
// own class files, so they can be included in other header files.

Layer::Layer(SpatialComputer* p) {
  parent = p;
  can_dump = p->is_dump_default;
}

// To create an initial distribution, we must know:
// - # nodes, volume they occupy, distribution type, any dist-specific args

Distribution::Distribution(int n, Rect *volume) { // subclasses often take an Args* too
  this->n=n; this->volume=volume;
  width = volume->r-volume->l; height = volume->t-volume->b; depth=0;
  if(volume->dimensions()==3) depth=((Rect3*)volume)->c-((Rect3*)volume)->f;
}

// puts location in *loc (a 3-vector) and returns to make a new device
BOOL Distribution::next_location(METERS *loc) { return FALSE; }
