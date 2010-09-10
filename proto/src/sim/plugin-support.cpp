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

DllNotFoundException::DllNotFoundException(string msg) :
  message(msg) {
}
DllNotFoundException::~DllNotFoundException() throw () {
}
const char* DllNotFoundException::what() const throw () {
  string returnStr = "Error: Library file for plugin or layer " + message + " not found.";
  return returnStr.c_str();
}
