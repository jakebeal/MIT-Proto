/* Example of stand-alone plugin development
   Distributed in the public domain.
*/

#include "ExamplePlugin.h"
#include "proto/proto_vm.h"

#define FOO_OP "foo boolean scalar"

/*************** FooDistribution ***************/
// create a spiral
FooDistribution::FooDistribution(int n, Rect* volume, Args* args) : Distribution(n,volume) {
  rad = args->extract_switch("-foo-rad")?args->pop_number():0.1;
  d = MIN(width,height)/n/2; i=0;
}
BOOL FooDistribution::next_location(METERS *loc) {
  loc[0] = cos(rad*i)*d*i; loc[1] = sin(rad*i)*d*i; loc[2] = 0;
  i++; // increment for next point
  return true; // yes, make the device
}


/*************** FooTimer ***************/
// run timer faster farther from the center

void FooTimer::next_transmit(SECONDS* d_true, SECONDS* d_internal) {
  const flo* pos = device->body->position();
  float dist = sqrt((pos[0]*pos[0])+(pos[1]*pos[1])+(pos[2]*pos[2]));
  *d_true = *d_internal = 1/(1+dist/k)/2;
}
void FooTimer::next_compute(SECONDS* d_true, SECONDS* d_internal) {
  const flo* pos = device->body->position();
  float dist = sqrt((pos[0]*pos[0])+(pos[1]*pos[1])+(pos[2]*pos[2]));
  *d_true = *d_internal = 1/(1+dist/k);
}

FooTime::FooTime(Args* args) {
  k = args->extract_switch("-foo-k")?args->pop_number():10;
  if(k<=0) uerror("Cannot specify non-positive time steps.");
}


/*************** FooLayer ***************/
// Cyclic timer
FooLayer::FooLayer(Args* args, SpatialComputer* p) : Layer(p) {
  post("Example plugin online!\n");
  this->parent=parent;
  cycle = args->extract_switch("-foo-cycle")?(int)args->pop_number():3;
  args->undefault(&can_dump,"-Dfoo","-NDfoo");
  // register hardware functions
  parent->hardware.registerOpcode(new OpHandler<FooLayer>(this, &FooLayer::foo_op, FOO_OP));
}

void FooLayer::foo_op(MACHINE* machine) {
  float want = NUM_POP(), val = -1;
  if(want) val = ((FooDevice*)device->layers[id])->foo_timer;
  NUM_PUSH(val);
}

// Called by the simulator to initialize a new device
void FooLayer::add_device(Device* d) {
  d->layers[id] = new FooDevice(this,d);
}

// Called when recording state to file
void FooLayer::dump_header(FILE* out) {
  if(can_dump) fprintf(out," \"FOOTIME\"");
}



FooDevice::FooDevice(FooLayer* parent,Device* d) : DeviceLayer(d) {
  this->parent=parent;
  foo_timer = parent->cycle;
}

void FooDevice::dump_state(FILE* out, int verbosity) {
  if(verbosity==0) { fprintf(out," %.2f", foo_timer);
  } else { fprintf(out,"Example plugin timer %.2f\n",foo_timer);
  }
}

void FooDevice::update() {
  foo_timer -= (machine->time - machine->last_time);
  if(foo_timer<=0) foo_timer += parent->cycle;
}

BOOL FooDevice::handle_key(KeyEvent* key) {
  // is this a key recognized internally?
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'B': foo_timer += 8; return TRUE;
    }
  }
  return FALSE;
}


/*************** Plugin Library ***************/
void* ExamplePlugin::get_sim_plugin(string type,string name,Args* args, 
                                            SpatialComputer* cpu, int n) {
  if(type == DISTRIBUTION_PLUGIN) {
    if(name == DIST_NAME) { return new FooDistribution(n,cpu->volume,args); }
  } else if(type == TIMEMODEL_PLUGIN) {
    if(name == TIME_NAME) { return new FooTime(args); }
  } else if(type == LAYER_PLUGIN) {
    if(name == LAYER_NAME) { return new FooLayer(args, cpu); }
  }
  return NULL;
}

string ExamplePlugin::inventory() {
  return "# Example plugin\n" +
    registry_entry(DISTRIBUTION_PLUGIN,DIST_NAME,DLL_NAME) +
    registry_entry(TIMEMODEL_PLUGIN,TIME_NAME,DLL_NAME) +
    registry_entry(LAYER_PLUGIN,LAYER_NAME,DLL_NAME);
}

extern "C" {
  ProtoPluginLibrary* get_proto_plugin_library() 
  { return new ExamplePlugin(); }
  const char* get_proto_plugin_inventory()
  { return (new string(ExamplePlugin::inventory()))->c_str(); }
}
