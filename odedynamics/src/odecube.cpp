#include "odeshapes.h"



Cube::Cube(ODEDynamics *parent, Device* container, flo x, flo y, flo z, flo r) : ODEBody(parent, container, x, y, z, r) {

}

//void Cube::visualize() {
//#ifdef WANT_GLUT
//  if(!parent->is_show_bot) return; // don't display unless should be shown
//  // get location/size information
//  const dReal pos[3] = {0,0,0};
//  const dReal *R = dGeomGetRotation(geom);
//  dVector3 sides; dGeomBoxGetLengths(geom,sides);
//
//  // choose base color
//  BOOL pushed=FALSE;
//  Color* color;
//  if (container->is_selected) {
//		palette->use_color(ODEDynamics::ODE_SELECTED);
//	} else {
//		if (!dBodyIsEnabled(body)) {
//			palette->use_color(ODEDynamics::ODE_DISABLED);
//		} else {
//			if (parent->is_multicolored_bots) {
//				flo h = (360.0 * parloc) / parent->bodies.max_id();
//				flo r, g, b;
//				hsv_to_rgb(h, 1, 1, &r, &g, &b);
//
//				//TODO was this to indicate boxes collided?
////				Color* c = palette->register_color("a",r/255,g/255,b/255,0.7);
////				palette->use_color(c);
//
//				pushed = TRUE;
//			}
//		}
//	}
//  if(did_bump){
//		palette->use_color(ODEDynamics::ODE_BOT_BUMPED);
//  }
//  draw_box(pos,R,sides);
//  palette->use_color(ODEDynamics::ODE_EDGES); // draw edges in separate color
//  draw_wire_box(pos,R,sides);
//  if(pushed) palette->use_color(ODEDynamics::ODE_BOT);
//#endif // WANT_GLUT
//}
