/* Example of stand-alone plugin development
   Distributed in the public domain.
 */

#include <proto/proto_plugin.h>
#include <proto/spatialcomputer.h>
#define DIST_NAME "foo-dist"
#define TIME_NAME "foo-time"
#define LAYER_NAME "foo-layer"
#define DLL_NAME "libexampleplugin"

// Foo Distribution puts points in a spiral out from the center
class FooDistribution : public Distribution {
 public:
  float rad; float d; int i;
  FooDistribution(int n, Rect* volume, Args* args);
  BOOL next_location(METERS *loc);
};

// FooTimer runs faster farther from the center
class FooTimer : public DeviceTimer {
 public:
  float k; // stretch factor on acceleration
  FooTimer(float k) { this->k = k; }
  void next_transmit(SECONDS* d_true, SECONDS* d_internal);
  void next_compute(SECONDS* d_true, SECONDS* d_internal);
  DeviceTimer* clone_device() { return new FooTimer(k); };
};

class FooTime : public TimeModel {
 public:
  float k;
  FooTime(Args* args);
  DeviceTimer* next_timer(SECONDS* start_lag) { return new FooTimer(k); }
  SECONDS cycle_time() { return 1; }
};

// The FooLayer provides a sensor for a cyclic timer
class FooLayer : public Layer, public HardwarePatch {
 public:
  float cycle;
  FooLayer(Args* args, SpatialComputer* parent);
  void add_device(Device* d);
  void dump_header(FILE* out); // list log-file fields
 private:
  void foo_op(MACHINE* machine);
};

class FooDevice : public DeviceLayer {
 public:
  FooLayer* parent;
  float foo_timer;
  FooDevice(FooLayer* parent, Device* container);
  void update();
  BOOL handle_key(KeyEvent* event);
  void copy_state(DeviceLayer* src) {} // to be called during cloning
  void dump_state(FILE* out, int verbosity); // print state to file
};


class ExamplePlugin : public ProtoPluginLibrary {
public:
  void* get_sim_plugin(string type, string name, Args* args, 
                       SpatialComputer* cpu, int n);
  static string inventory();
};

