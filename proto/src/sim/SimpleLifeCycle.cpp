/* 
 * File:   SimpleLifeCycle.cpp
 * Author: prakash
 * 
 * Created on February 8, 2010, 12:27 PM
 */

#include "SimpleLifeCycle.h"

/*****************************************************************************
 *  SIMPLE LIFECYCLE                                                         *
 *****************************************************************************/
// instant death, cloning after a delay
SimpleLifeCycle::SimpleLifeCycle(Args* args, SpatialComputer* p) : Layer(p) {
  this->parent=parent;
  clone_delay = args->extract_switch("-clone-delay")?(int)args->pop_number():1;
  args->undefault(&can_dump,"-Dclone","-NDclone");
  // register hardware functions
  parent->hardware.patch(this,DIE_FN);
  parent->hardware.patch(this,CLONE_MACHINE_FN);
}

void SimpleLifeCycle::add_device(Device* d) {
  d->layers[id] = new SimpleLifeCycleDevice(this,d);
}

void SimpleLifeCycle::die (NUM_VAL val) {
  if(val != 0) parent->death_q.push(device->backptr);
}
void SimpleLifeCycle::clone_machine (NUM_VAL val) {
  if(val != 0) ((SimpleLifeCycleDevice*)device->layers[id])->clone_cmd=TRUE;
}

void SimpleLifeCycle::dump_header(FILE* out) {
  if(can_dump) fprintf(out," \"CLONETIME\"");
}



SimpleLifeCycleDevice::SimpleLifeCycleDevice(SimpleLifeCycle* parent,
                                             Device* d) : DeviceLayer(d) {
  this->parent=parent;
  clone_timer=parent->clone_delay; clone_cmd=FALSE; // start unready to clone
}

void SimpleLifeCycleDevice::dump_state(FILE* out, int verbosity) {
  if(verbosity==0) { fprintf(out," %.2f", clone_timer);
  } else { fprintf(out,"Time to cloning %.2f\n",clone_timer);
  }
}

// choose new position w. random polar coordinates
void SimpleLifeCycleDevice::clone_me() {
  const flo* p = container->body->position();
  flo dp = 2*container->body->display_radius();
  flo theta = urnd(0,2*M_PI);
  flo phi = (parent->parent->is_3d() ? urnd(0,2*M_PI) : 0);
  METERS cp[3];
  cp[0] = p[0] + dp*cos(phi)*cos(theta);
  cp[1] = p[1] + dp*cos(phi)*sin(theta);
  cp[2] = p[2] + dp*sin(phi);
  CloneReq* cr = new CloneReq(container->backptr,container,cp);
  parent->parent->clone_q.push(cr);
}

void SimpleLifeCycleDevice::update() {
  if(clone_cmd) {
    clone_cmd=FALSE;
    if(clone_timer>0) { clone_timer -= (machine->time - machine->last_time); }
    if(clone_timer<=0) {
      clone_me();
      while(clone_timer<=0) {clone_timer+=parent->clone_delay;} // reset timer
    }
  }
}

BOOL SimpleLifeCycleDevice::handle_key(KeyEvent* key) {
  // is this a key recognized internally?
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'B': clone_me(); return TRUE;
    case 'K': parent->parent->death_q.push(container->backptr); return TRUE;
    }
  }
  return FALSE;
}



