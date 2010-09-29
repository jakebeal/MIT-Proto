/* Example of stand-alone plugin development
   Distributed in the public domain.
 */

#include <proto/proto_plugin.h>
#include <proto/spatialcomputer.h>
#define LAYER_NAME "foo-layer"
#define DLL_NAME "libexampleplugin"

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

