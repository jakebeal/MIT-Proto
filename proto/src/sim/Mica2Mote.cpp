#include "Mica2Mote.h"
#include <sstream>
using namespace std;
/*****************************************************************************
 *  TESTBED MOTE IO                                                          *
 *****************************************************************************/
MoteIO::MoteIO(Args* args, SpatialComputer* parent) : Layer(parent) {
  args->undefault(&can_dump,"-Dmoteio","-NDmoteio");
  // register patches
  parent->hardware.patch(this,SET_SPEAK_FN);
  parent->hardware.patch(this,READ_LIGHT_SENSOR_FN);
  parent->hardware.patch(this,READ_MICROPHONE_FN);
  parent->hardware.patch(this,READ_TEMP_FN);
  parent->hardware.patch(this,READ_SHORT_FN);
  parent->hardware.patch(this,READ_BUTTON_FN);
  parent->hardware.patch(this,READ_SLIDER_FN);
}
void MoteIO::add_device(Device* d) {
  d->layers[id] = new DeviceMoteIO(this,d);
}
BOOL MoteIO::handle_key(KeyEvent* event) {
  return FALSE; // right now, there's no keys that affect these globally
}

void MoteIO::dump_header(FILE* out) {
  if(can_dump) fprintf(out," \"SOUND\" \"TEMP\" \"BUTTON\"");
}

// hardware emulation
void MoteIO::set_speak (NUM_VAL period) {
  // right now, setting the speaker does *nothing*, as in the old sim
}
NUM_VAL MoteIO::read_light_sensor(VOID) { return machine->sensors[LIGHT]; }
NUM_VAL MoteIO::read_microphone (VOID) { return machine->sensors[SOUND]; }
NUM_VAL MoteIO::read_temp (VOID) { return machine->sensors[TEMPERATURE]; }
NUM_VAL MoteIO::read_short (VOID) { return 0; }
NUM_VAL MoteIO::read_button (uint8_t n) {
  return ((DeviceMoteIO*)device->layers[id])->button;
}
NUM_VAL MoteIO::read_slider (uint8_t ikey, uint8_t dkey, NUM_VAL init, 
			     NUM_VAL incr, NUM_VAL min, NUM_VAL max) {
  // slider is not yet implemented
}

void DeviceMoteIO::dump_state(FILE* out, int verbosity) {
  MACHINE* m = container->vm;
  if(verbosity==0) { 
    fprintf(out," %.2f %.2f %d", m->sensors[SOUND], m->sensors[TEMPERATURE], 
            button);
  } else { 
    fprintf(out,"Mic = %.2f, Temp = %.2f, Button = %s\n",m->sensors[SOUND], 
            m->sensors[TEMPERATURE], bool2str(button));
  }
}

// individual device implementations
BOOL DeviceMoteIO::handle_key(KeyEvent* key) {
  // I think that the slider is supposed to consume keys too
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'N': button = !button; return TRUE;
    }
  }
  return FALSE;
}

void DeviceMoteIO::visualize(Device* d) {
#ifdef WANT_GLUT
  flo rad = d->body->display_radius();
  // draw the button being on
  if (button) {
    palette->use_color(BUTTON_COLOR);
    draw_disk(rad*SENSOR_RADIUS_FACTOR);
  }
#endif // WANT_GLUT
}

Mica2MotePlugin::Mica2MotePlugin()
{
    layerName = MICA2MOTE_NAME;
}
Layer* Mica2MotePlugin::get_layer(char* name, Args* args,SpatialComputer* cpu, int n)
{
    if(layerName == string(name))
    {
        new MoteIO(args,cpu);
    }
    return NULL;
}

string Mica2MotePlugin::getProperties()
{        
    stringstream ss;
    ss << "Layer " << MICA2MOTE_NAME << " = " << MICA2MOTE_DLL_NAME << endl;
    return ss.str();    
}

#ifdef __cplusplus

extern "C" {

ProtoPluginLibrary* get_proto_plugins()
{
    return new Mica2MotePlugin();
}
const char* get_proto_plugin_properties()
{
    string propS = Mica2MotePlugin::getProperties();
    char *props = new char[propS.size() + 1];
    strncpy(props, propS.c_str(), propS.size());
    props[propS.size()] = '\0';
    return props;
}

}
#endif
