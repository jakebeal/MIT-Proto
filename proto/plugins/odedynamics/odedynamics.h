/* Newtonian physics simulation using the Open Dynamics Engine
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __ODEDYNAMICS__
#define __ODEDYNAMICS__

#include <proto/spatialcomputer.h>
#include <ode/ode.h>

/*****************************************************************************
 *  ODE BODY                                                                 *
 *****************************************************************************/
class ODEDynamics;

class ODEBody : public Body {
 public:
  ODEDynamics* parent; int parloc; // back pointers
  dBodyID body; dGeomID geom;      // the basic ODEBody is always a cube
  flo       did_bump;              // bump sensor
  flo       desired_v[3];   // the velocity that's wanted
  
  ODEBody(ODEDynamics *parent, Device* container, flo x, flo y, flo z, flo r);
  ~ODEBody();
  void visualize();
  void render_selection();
  void preupdate() { for(int i=0;i<3;i++) desired_v[i]=0; }
  void update() { did_bump=false; }
  void dump_state(FILE* out, int verbosity); // print state to file
  void drive(); // internal: called to apply force toward the desired velocity
  
  // accessors
  const flo* position() { return dBodyGetPosition(body); }
  const flo* orientation() { return dBodyGetQuaternion(body); }
  const flo* velocity() { return dBodyGetLinearVel(body); }
  const flo* ang_velocity() { return dBodyGetAngularVel(body); }
  void set_position(flo x, flo y, flo z) { dBodySetPosition(body,x,y,z); }
  void set_orientation(const flo *q) { dBodySetQuaternion(body,q); }
  void set_velocity(flo dx, flo dy, flo dz) 
    { dBodySetLinearVel(body,dx,dy,dz); }
  void set_ang_velocity(flo dx, flo dy, flo dz) 
    { dBodySetAngularVel(body,dx,dy,dz); }
  flo display_radius() 
    { dVector3 len; dGeomBoxGetLengths(geom,len); return len[0]; }
};

/*****************************************************************************
 *  ODE DYNAMICS                                                             *
 *****************************************************************************/

#define ODE_N_WALLS 8
class ODEDynamics : public BodyDynamics, HardwarePatch {
  static BOOL inited;  // Has dInitODE been called yet?
 public:
  Population bodies;
  dWorldID  world;            // ODE body storage
  dSpaceID  space;            // ODE collision support
  dJointGroupID contactgroup; // ODE body motion constraints
  dGeomID walls[ODE_N_WALLS];
  dGeomID pen;                // the pen, for testing for escapes

  BOOL is_show_bot;         // display this layer at all
  BOOL is_mobile;           // evolve layer forward
  BOOL is_walls;            // put force boundaries on area
  BOOL is_draw_walls;       // should the walls be seen
  BOOL is_inescapable;      // bodies are reset within walls when they escape
  BOOL is_multicolored_bots;  // draw bots rainbow colored!
  flo speed_lim;             // maximum speed (defaults to infinity)
  flo body_radius, density; // default body parameters
  flo substep, time_slop;   // managing multiple substeps per step
  
  ODEDynamics(Args* args, SpatialComputer* parent,int n);
  ~ODEDynamics();
  BOOL evolve(SECONDS dt);
  BOOL handle_key(KeyEvent* key);
  void visualize();
  Body* new_body(Device* d, flo x, flo y, flo z);
  void dump_header(FILE* out); // list log-file fields

  // hardware emulation
  void mov(VEC_VAL *val);
  NUM_VAL read_bump (VOID);
  // not yet implemented:
  //NUM_VAL radius_set (NUM_VAL val);
  //NUM_VAL radius_get (VOID);
  //VEC_VAL *read_coord_sensor(VOID);
  //VEC_VAL *read_ranger (VOID);
  //NUM_VAL read_bearing (VOID);
  //NUM_VAL read_speed (VOID);
  layer_type get_type();
 private:
  void make_walls();
  void reset_escapes(); // used when the walls are inescapable
  void ODEDynamics::bump_op(MACHINE* machine);
};

#endif //__ODEDYNAMICS__
