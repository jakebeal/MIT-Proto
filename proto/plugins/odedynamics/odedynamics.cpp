/* Newtonian physics simulation using the Open Dynamics Engine
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include "odedynamics.h"
#include <proto/visualizer.h>

/*****************************************************************************
 *  GEOM DRAWING                                                             *
 *****************************************************************************/
// Note: this largely duplicates some of the code in drawing_primitives.cpp

static void set_transform(const float pos[3], const float R[12]) {
#ifdef WANT_GLUT
  GLfloat matrix[16];
  matrix[0]=R[0];    matrix[1]=R[4];    matrix[2]=R[8];    matrix[3]=0;
  matrix[4]=R[1];    matrix[5]=R[5];    matrix[6]=R[9];    matrix[7]=0;
  matrix[8]=R[2];    matrix[9]=R[6];    matrix[10]=R[10];  matrix[11]=0;
  matrix[12]=pos[0]; matrix[13]=pos[1]; matrix[14]=pos[2]; matrix[15]=1;
  glPushMatrix();
  glMultMatrixf(matrix);
#endif // WANT_GLUT
}

static void do_draw_wire_box(const float sides[3]) {
#ifdef WANT_GLUT
  float w = sides[0]/2, h = sides[1]/2, d = sides[2]/2;
  glBegin( GL_LINE_STRIP );
  // top
  glVertex3d(-w, -h, -d);
  glVertex3d(-w,  h, -d);
  glVertex3d( w,  h, -d);
  glVertex3d( w, -h, -d);
  glVertex3d(-w, -h, -d);
  // bot
  glVertex3d(-w, -h,  d);
  glVertex3d(-w,  h,  d);
  glVertex3d( w,  h,  d);
  glVertex3d( w, -h,  d);
  glVertex3d(-w, -h,  d);
  // sides
  glVertex3d( w, -h,  d);
  glVertex3d( w, -h, -d);
  glVertex3d( w,  h, -d);
  glVertex3d( w,  h,  d);
  glVertex3d(-w,  h,  d);
  glVertex3d(-w,  h, -d);
  glEnd();
#endif // WANT_GLUT
}


static void do_draw_box(const float sides[3]) {
#ifdef WANT_GLUT
  float lx = sides[0]*0.5f, ly = sides[1]*0.5f, lz = sides[2]*0.5f;
  // sides
  glBegin (GL_TRIANGLE_STRIP);
  glNormal3f (-1,0,0);
  glVertex3f (-lx,-ly,-lz);
  glVertex3f (-lx,-ly,lz);
  glVertex3f (-lx,ly,-lz);
  glVertex3f (-lx,ly,lz);
  glNormal3f (0,1,0);
  glVertex3f (lx,ly,-lz);
  glVertex3f (lx,ly,lz);
  glNormal3f (1,0,0);
  glVertex3f (lx,-ly,-lz);
  glVertex3f (lx,-ly,lz);
  glNormal3f (0,-1,0);
  glVertex3f (-lx,-ly,-lz);
  glVertex3f (-lx,-ly,lz);
  glEnd();
  // top face
  glBegin (GL_TRIANGLE_FAN);
  glNormal3f (0,0,1);
  glVertex3f (-lx,-ly,lz);
  glVertex3f (lx,-ly,lz);
  glVertex3f (lx,ly,lz);
  glVertex3f (-lx,ly,lz);
  glEnd();
  // bottom face
  glBegin (GL_TRIANGLE_FAN);
  glNormal3f (0,0,-1);
  glVertex3f (-lx,-ly,-lz);
  glVertex3f (-lx,ly,-lz);
  glVertex3f (lx,ly,-lz);
  glVertex3f (lx,-ly,-lz);
  glEnd();
#endif // WANT_GLUT
}

void draw_box(const float *pos, const float *R, const float *sides) {
#ifdef WANT_GLUT
  set_transform(pos, R);
  do_draw_box(sides);
  glPopMatrix();
#endif // WANT_GLUT
}

void draw_wire_box(const float *pos, const float *R, const float *sides) {
#ifdef WANT_GLUT
  set_transform(pos, R);
  do_draw_wire_box(sides);
  glPopMatrix();
#endif // WANT_GLUT
}

/*****************************************************************************
 *  ODE BODY                                                                 *
 *****************************************************************************/

ODEBody::ODEBody(ODEDynamics *parent, Device* container, flo x, flo y, flo z, 
                 flo r) : Body(container) {
  this->parent=parent; moved=FALSE;
  for(int i=0;i<3;i++) desired_v[i]=0; 
  did_bump=false; 
  // create and attach body, shape, and mass
  body = dBodyCreate(parent->world);
  geom = dCreateBox(parent->space,r*2,r*2,r*2); 
  dMass m; dMassSetBox(&m,parent->density,r*2,r*2,r*2);
  dGeomSetBody(geom,body); dBodySetMass(body,&m);
  // set position and orientation
  dBodySetPosition(body, x, y, z);
  dQuaternion Q; dQFromAxisAndAngle (Q,0,0,1,0);
  dBodySetQuaternion (body,Q);
  // set up back-pointer
  dBodySetData(body,(void*)this);
  dGeomSetData(geom,(void*)this);
}

ODEBody::~ODEBody() {
  dBodyDestroy(body); dGeomDestroy(geom);
  parent->bodies.remove(parloc);
}

void ODEBody::visualize() {
#ifdef WANT_GLUT
  if(!parent->is_show_bot) return; // don't display unless should be shown

  // get location/size information
  const flo pos[3] = {0,0,0};
  const dReal *R = dGeomGetRotation(geom);
  dVector3 sides; dGeomBoxGetLengths(geom,sides);
  
  // choose base color
  BOOL pushed=FALSE;
  if(container->is_selected) { palette->use_color(ODE_SELECTED);
  } else if(!dBodyIsEnabled(body)) { palette->use_color(ODE_DISABLED);
  } else {
    if(parent->is_multicolored_bots) {
      flo h = (360.0*parloc)/parent->bodies.max_id();
      flo r, g, b; hsv_to_rgb(h, 1, 1, &r, &g, &b);
      palette->push_color(ODE_BOT,r/255,g/255,b/255,0.7);
      pushed=TRUE;
    }
    palette->use_color(ODE_BOT);
  }
  draw_box(pos,R,sides);
  palette->use_color(ODE_EDGES); // draw edges in separate color
  draw_wire_box(pos,R,sides);
  if(pushed) palette->pop_color(ODE_BOT);
#endif // WANT_GLUT
}

void ODEBody::render_selection() {
  const flo pos[3] = {0,0,0};
  const dReal *R = dGeomGetRotation(geom);
  dVector3 sides; dGeomBoxGetLengths(geom,sides);
  draw_box(pos,R,sides);  
}

void ODEBody::dump_state(FILE* out, int verbosity) {
  if(verbosity==0) {
    const flo *v = position(); fprintf(out," %.2f %.2f %.2f",v[0],v[1],v[2]);
    v = velocity(); fprintf(out," %.2f %.2f %.2f",v[0],v[1],v[2]);
    v = orientation(); fprintf(out," %.2f %.2f %.2f %.2f",v[0],v[1],v[2],v[3]);
    v = ang_velocity(); fprintf(out," %.2f %.2f %.2f",v[0],v[1],v[2]);
  } else {
    const flo *v = position();
    fprintf(out,"Position=[%.2f %.2f %.2f], ",v[0],v[1],v[2]);
    v = velocity();
    fprintf(out,"Velocity=[%.2f %.2f %.2f] (Speed=%.2f)\n",v[0],v[1],v[2],
            sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]));
    v = orientation();
    fprintf(out,"Orientation=[%.2f %.2f %.2f %.2f], ",v[0],v[1],v[2],v[3]);
    v = ang_velocity();
    fprintf(out,"Angular Velocity=[%.2f %.2f %.2f]\n",v[0],v[1],v[2]);
  }
}

#define K_MOVE 10
void ODEBody::drive() {
  const dReal *cur = velocity();
  dReal force[3];
  force[0] = K_MOVE * (desired_v[0] - cur[0]);
  force[1] = K_MOVE * (desired_v[1] - cur[1]);
  force[2] = (parent->parent->is_3d())? K_MOVE * (desired_v[2] - cur[2]) : 0;
  dBodyAddForce(body, force[0], force[1], force[2]);
}

/*****************************************************************************
 *  COLLISION HANDLING                                                       *
 *****************************************************************************/

#define WALL_DATA -1            // identifier for walls
#define MAX_CONTACTS 8		// maximum number of contact points per body

static int isWall (dGeomID g) { return dGeomGetData(g)==((void*)WALL_DATA); }

static void nearCallback (void *data, dGeomID o1, dGeomID o2) {
  int i, numc;
  ODEDynamics *dyn = (ODEDynamics*)data;

  // exit without doing anything if the two bodies are connected by a joint
  dBodyID b1 = dGeomGetBody(o1); dBodyID b2 = dGeomGetBody(o2);
  if (b1 && b2 && dAreConnectedExcluding(b1,b2,dJointTypeContact)) return;
  // exit without doing anything if the walls are off and one object is a wall
  if(!dyn->is_walls && (isWall(o1) || isWall(o2))) { return; }

  dContact contact[MAX_CONTACTS];   // up to MAX_CONTACTS contacts per pair
  if (int numc = dCollide (o1,o2,MAX_CONTACTS,&contact[0].geom,
                           sizeof(dContact))) {
    for (i=0; i<numc; i++) {
      contact[i].surface.mode = dContactBounce | dContactSoftCFM;
      contact[i].surface.mu = 0;
      // contact[i].surface.mu = dInfinity;
      contact[i].surface.mu2 = 0;
      // contact[i].surface.bounce = 0.1;
      contact[i].surface.bounce = 0.5;
      contact[i].surface.bounce_vel = 0.1;
      contact[i].surface.soft_cfm = 0.0; // give
      dJointID c=dJointCreateContact(dyn->world,dyn->contactgroup,&contact[i]);
      dJointAttach(c,b1,b2);
    }
    // record bumps
    if(b1 && (b2 || isWall(o2))) ((ODEBody*)dBodyGetData(b1))->did_bump=TRUE;
    if(b2 && (b1 || isWall(o1))) ((ODEBody*)dBodyGetData(b2))->did_bump=TRUE;
  }
}

/*****************************************************************************
 *  ODE DYNAMICS                                                             *
 *****************************************************************************/

#define DENSITY (0.125/2)       // default density
#define MAX_V 100               // default ceiling on velocity
#define SUBSTEP 0.001           // default sub-step size
#define K_BODY_RAD 0.0870 // constant matched against previous visualization

void ODEDynamics::make_walls() {
  flo pen_w=parent->volume->r - parent->volume->l;
  flo pen_h=parent->volume->t - parent->volume->b;
  flo wall_width = 5;
  dQuaternion Q;
  
  pen = dCreateBox(0,pen_w+2*wall_width,pen_h+2*wall_width,10*pen_h);
  
  // side walls
  dQFromAxisAndAngle (Q,0,0,1,M_PI/2);
  walls[0] = dCreateBox(space, pen_h+2*wall_width, wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[0],Q);
  dGeomSetPosition (walls[0], pen_w/2+wall_width/2, 0, 0);
  dGeomSetData(walls[0], (void*)WALL_DATA);
  
  walls[1] = dCreateBox(space, pen_h+2*wall_width, wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[1],Q);
  dGeomSetPosition (walls[1], -pen_w/2-wall_width/2, 0, 0);
  dGeomSetData(walls[1], (void*)WALL_DATA);
  
  dQFromAxisAndAngle (Q,0,0,1,0);
  walls[2] = dCreateBox(space, pen_w+2*wall_width, wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[2],Q);
  dGeomSetPosition (walls[2], 0, pen_h/2+wall_width/2, 0);
  dGeomSetData(walls[2], (void*)WALL_DATA);
  
  walls[3] = dCreateBox(space, pen_w+2*wall_width, wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[3],Q);
  dGeomSetPosition (walls[3], 0, -pen_h/2-wall_width/2, 0);
  dGeomSetData(walls[3], (void*)WALL_DATA);

  // corner walls
  dQFromAxisAndAngle (Q,0,0,1,M_PI/4);

  walls[4] = dCreateBox(space, 2*wall_width, 2*wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[4],Q);
  dGeomSetPosition (walls[4], -pen_w/2, -pen_h/2, 0);
  dGeomSetData(walls[4], (void*)WALL_DATA);

  walls[5] = dCreateBox(space, 2*wall_width, 2*wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[5],Q);
  dGeomSetPosition (walls[5],  pen_w/2, -pen_h/2, 0);
  dGeomSetData(walls[5], (void*)WALL_DATA);

  walls[6] = dCreateBox(space, 2*wall_width, 2*wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[6],Q);
  dGeomSetPosition (walls[6],  pen_w/2,  pen_h/2, 0);
  dGeomSetData(walls[6], (void*)WALL_DATA);

  walls[7] = dCreateBox(space, 2*wall_width, 2*wall_width, 10*pen_h);
  dGeomSetQuaternion (walls[7],Q);
  dGeomSetPosition (walls[7], -pen_w/2,  pen_h/2, 0);
  dGeomSetData(walls[7], (void*)WALL_DATA);
}

// Note: ODE's manual claims that dCloseODE is optional
BOOL ODEDynamics::inited = FALSE;
ODEDynamics::ODEDynamics(Args* args, SpatialComputer* p, int n)
  : BodyDynamics(p) {
  if(!inited) { dInitODE(); inited=TRUE; } // initialize if not yet done so
  time_slop=0;
  flo width=parent->volume->r - parent->volume->l;
  flo height=parent->volume->t - parent->volume->b;
  // read options
  body_radius = (args->extract_switch("-rad")) ? args->pop_number()
    : K_BODY_RAD*sqrt((width*height)/(flo)n);
  density = (args->extract_switch("-density")) ? args->pop_number() : DENSITY;
  speed_lim = (args->extract_switch("-S"))?args->pop_number():MAX_V;
  substep = (args->extract_switch("-substep"))?args->pop_number() : SUBSTEP;
  if(substep <= 0) { 
    post("-substep must be greater than zero; using default %f",SUBSTEP);
    substep = SUBSTEP;
  }
  is_mobile = args->extract_switch("-m");
  is_show_bot = !args->extract_switch("-hide-body");
  is_walls = ((args->extract_switch("-w") & !args->extract_switch("-nw")))
    ? TRUE : FALSE;
  is_draw_walls = args->extract_switch("-draw-walls");
  is_inescapable = args->extract_switch("-inescapable");
  is_multicolored_bots = args->extract_switch("-rainbow-bots");
  args->undefault(&can_dump,"-Ddynamics","-NDdynamics");
  // register to simulate hardware
  parent->hardware.patch(this,MOV_FN);
  parent->hardware.registerOpcode(new OpHandler<ODEDynamics>("BUMP_OP", this, &ODEDynamics::bump_op, "bump boolean"));
  // Initialize ODE and make the walls
  world = dWorldCreate();
  space = dHashSpaceCreate(0);
  contactgroup = dJointGroupCreate(0);
  if(p->volume->dimensions()==2) dWorldSetGravity(world,0,0,-9.81);
  dWorldSetCFM(world,1e-5);
  dWorldSetAutoDisableFlag(world,0); // nothing ever disables
  dWorldSetAutoDisableAverageSamplesCount(world, 10);
  dWorldSetContactMaxCorrectingVel(world,0.1);
  dWorldSetContactSurfaceLayer(world,0.001);
  if(p->volume->dimensions()==2) dCreatePlane(space,0,0,1,-body_radius);
  make_walls();
}

ODEDynamics::~ODEDynamics() {
  dJointGroupDestroy(contactgroup);
  for(int i=0;i<ODE_N_WALLS;i++) dGeomDestroy(walls[i]);
  dGeomDestroy(pen);
  dSpaceDestroy(space);
  dWorldDestroy(world);
}

void ODEDynamics::bump_op(MACHINE* machine) {
  NUM_PUSH(read_bump());
}

BOOL ODEDynamics::handle_key(KeyEvent* key) {
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'w': is_walls = !is_walls; return TRUE;
    case 'b': is_show_bot = !is_show_bot; return TRUE;
    case 'm': is_mobile = !is_mobile; return TRUE;
    }
  }
  return FALSE;
}

void ODEDynamics::visualize() {
#ifdef WANT_GLUT
  if(is_walls && is_draw_walls) {
    for(int i=0; i<ODE_N_WALLS; i++) {
      const dReal *pos = dGeomGetPosition(walls[i]);
      const dReal *R = dGeomGetRotation(walls[i]);
      dVector3 sides; dGeomBoxGetLengths(walls[i],sides);
      palette->use_color(ODE_WALL);
      draw_box(pos,R,sides);
      palette->use_color(ODE_EDGES);
      draw_wire_box(pos,R,sides);
    }
  }
#endif // WANT_GLUT
}

Body* ODEDynamics::new_body(Device* d, flo x, flo y, flo z) {
  ODEBody* b = new ODEBody(this,d,x,y,z,body_radius);
  b->parloc = bodies.add(b);
  return b;
}

void ODEDynamics::dump_header(FILE* out) {
  if(can_dump) {
    fprintf(out," \"X\" \"Y\" \"Z\"");
    fprintf(out," \"V_X\" \"V_Y\" \"V_Z\"");
    fprintf(out," \"Q_X\" \"Q_Y\" \"Q_Z\" \"Q_W\"");
    fprintf(out," \"W_X\" \"W_Y\" \"W_Z\"");
  }
}

BOOL ODEDynamics::evolve(SECONDS dt) {
  if(!is_mobile) return FALSE;
  time_slop+=dt;
  while(time_slop>0) {
    dSpaceCollide(space,this,&nearCallback);
    // add forces
    for(int i=0;i<bodies.max_id();i++) 
      { ODEBody* b = (ODEBody*)bodies.get(i); if(b) b->drive(); }
    dWorldQuickStep(world,substep);
    dJointGroupEmpty(contactgroup);
    if(is_walls && is_inescapable) reset_escapes();
    time_slop -= substep;
  }
  for(int i=0;i<bodies.max_id();i++) // mark everything as moved
    { Body* b = (Body*)bodies.get(i); if(b) b->moved=TRUE; }
  return TRUE;
}

// return escaped bots to a new starting position
void ODEDynamics::reset_escapes() {
  for(int i=0;i<bodies.max_id();i++) { 
    ODEBody* b = (ODEBody*)bodies.get(i); 
    if(b) {
      dContact contact;
      if(dCollide(pen,b->geom,1,&contact.geom,sizeof(dContact))) continue;
      // if not in pen, reset
      const flo *p = b->position();
      //post("You cannot escape! [%.2f %.2f %.2f]\n",p[0],p[1],p[2]);
      METERS loc[3];
      if(parent->distribution->next_location(loc)) {
        b->set_position(loc[0],loc[1],loc[2]);
        b->set_velocity(0,0,0); b->set_ang_velocity(0,0,0); // start still
      } else { // if there's no starting position available, die instead
        hardware->set_vm_context(b->container);
        die(TRUE);
      }
    }
  }
}

// hardware emulation
void ODEDynamics::mov(VEC_VAL *v) {
  ODEBody* b = (ODEBody*)device->body;
  b->desired_v[0] = NUM_GET(&v->elts[0]);
  b->desired_v[1] = NUM_GET(&v->elts[1]);
  b->desired_v[2] = v->n > 2 ? NUM_GET(&v->elts[2]) : 0.0;
  dReal len = sqrt(b->desired_v[0]*b->desired_v[0] + 
                   b->desired_v[1]*b->desired_v[1] +
                   b->desired_v[2]*b->desired_v[2]);
  if(len>speed_lim) {
    for(int i=0;i<3;i++) b->desired_v[i] *= speed_lim/len;
  }
}

NUM_VAL ODEDynamics::read_bump (VOID) {
  return (float)((ODEBody*)device->body)->did_bump;
}

extern "C" Layer *get_layer(Args *args, SpatialComputer *cpu, int n);

Layer *get_layer(Args *args, SpatialComputer *cpu, int n) {
  return new ODEDynamics(args, cpu, n);
}

layer_type ODEDynamics::get_type() {
  return LAYER_PHYSICS;
}
