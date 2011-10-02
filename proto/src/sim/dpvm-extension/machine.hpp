
void platform_operation(Int8); // This will be called for any unknown opcode.

enum Actuator {
	R_LED,
	G_LED,
	B_LED,
	N_ACTUATORS
};

enum Sensor {
	LIGHT,
	SOUND,
	TEMPERATURE,
	USER_A,
	USER_B,
        USER_C,
        USER_D,
	N_SENSORS
};

class SimMachine : public Machine {
	
	public:
		Number actuators[N_ACTUATORS];
		Number sensors[N_SENSORS];
		
	protected:
		void execute_unknown(Int8 opcode) {
			platform_operation(opcode);
		}
		
};

#undef Machine
#define Machine SimMachine
