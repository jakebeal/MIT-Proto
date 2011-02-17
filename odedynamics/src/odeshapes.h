#ifndef __ODESHAPES__
#define __ODESHAPES__

#include "odebody.h"
#include "odedynamics.h"
#include <proto/visualizer.h>

/**
 * Currently we support only the simple geometry objects
 * Support for Rays, Trimesh, Convex, and Heightfield geoms to be added
 * Additionally we want to add support for congregate geoms
 *
 * Currently supported geoms:
 * Sphere
 * Box
 * Capsule
 * Cylinder
 */
enum ShapeType { SPHERE = 0, BOX = 1, CAPSULE = 2, CYLINDER = 3};


class Cube : ODEBody{

	Cube(ODEDynamics *parent, Device* container, flo x, flo y, flo z, flo r);
	void visualize();


};


#endif //__ODESHAPES__
