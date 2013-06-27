var simulatorSettings = {
    numDevices : 100,
    radius : 10,
    stadiumSize : { x:100, y:100, z:100 },
    material : new THREE.MeshBasicMaterial( { color: 0xCC0000 }),
    deviceShape : new THREE.CubeGeometry(1,1,1),
    stepSize : 1.0,
    startPaused : false,
    startTime : 0.0,
    distribution : function(mid) {
       return {
          x : (Math.random() * simulatorSettings.stadiumSize.x), 
          y : (Math.random() * simulatorSettings.stadiumSize.y), 
          z : (Math.random() * simulatorSettings.stadiumSize.z)
       };
    },
    stopWhen : function(time) {
        return false;
    },
    preInitHook : null,
    deviceInitHook : null,
    preUpdateHook : null,
    deviceExecuteHook : null
};

function SpatialComputer() {
    this.devices = new Array();
    this.time = 0.0;
    this.init = function() {
      if(simulatorSettings.preInitHook) { simulatorSettings.preInitHook(); }

       this.time = simulatorSettings.startTime;

       // initialize the devices
       for(mid = 0; mid < simulatorSettings.numDevices; mid++) {

          this.devices[mid] = new THREE.Mesh(simulatorSettings.deviceShape, simulatorSettings.material);

          // set it's initial position
          this.devices[mid].position = simulatorSettings.distribution(mid);

          // add the sphere to the scene
          scene.add(this.devices[mid]);

          // initialize the Proto VM
          this.devices[mid].machine = new Module.Machine(
                                                         this.devices[mid].position.x, 
                                                         this.devices[mid].position.y, 
                                                         this.devices[mid].position.z, 
                                                         script);

          this.devices[mid].machine.resetActuators = function() {
             this.dx = 0;
             this.dy = 0;
             this.dz = 0;
          }

          // Set the device's internal time
          this.devices[mid].time = this.time;
          
          // Set the device's model of time
          this.devices[mid].deviceTimer = {
             nextTransmit : function(currentTime) { return 0.5 + currentTime; },
             nextCompute : function(currentTime) { return 1 + currentTime; }
          };
          
          this.devices[mid].nextTransmitTime = 
            this.devices[mid].deviceTimer.nextTransmit(this.time);
          this.devices[mid].nextComputeTime = 
            this.devices[mid].deviceTimer.nextCompute(this.time);

          this.devices[mid].requestDeath = false;
          this.devices[mid].requestClone = false;
          
          if(simulatorSettings.deviceInitHook) { simulatorSettings.deviceInitHook(this.device[mid]); }
       }
    };

    this.update = function() {
      if(simulatorSettings.preUpdateHook) { simulatorSettings.preUpdateHook(); }

       for(mid=0; mid < simulatorSettings.numDevices; mid++) {

          // if the machine should execute this timestep
          if(this.time >= this.devices[mid].nextComputeTime) {
             // zero-out all actuators
             this.devices[mid].machine.resetActuators();

             // while (!machine.finished()) machine.step();
             this.devices[mid].machine.executeRound(this.time);

             // update device time, etc...
             this.devices[mid].time = this.time;
             this.devices[mid].nextTransmitTime = 
                this.devices[mid].deviceTimer.nextTransmit(this.time);
             this.devices[mid].nextComputeTime = 
                this.devices[mid].deviceTimer.nextCompute(this.time);
             
             // call the deviceExecuteHook
             if(simulatorSettings.deviceExecuteHook) { simulatorSettings.deviceExecuteHook(this.devices[mid]); }
          }

          // update the color of the device
          this.devices[mid].material.color.r = (this.devices[mid].machine.red / 255);
          this.devices[mid].material.color.g = (this.devices[mid].machine.green / 255);
          this.devices[mid].material.color.b = (this.devices[mid].machine.blue / 255);

          // update the position of the device
          this.devices[mid].position = {
             x: this.devices[mid].machine.x + (this.devices[mid].machine.dx),
             y: this.devices[mid].machine.y + (this.devices[mid].machine.dy),
             z: this.devices[mid].machine.z + (this.devices[mid].machine.dz)
          };

          // update the position of the Proto VM
          this.devices[mid].machine.x = this.devices[mid].position.x;
          this.devices[mid].machine.y = this.devices[mid].position.y;
          this.devices[mid].machine.z = this.devices[mid].position.z;

       }

       this.time = this.time + simulatorSettings.stepSize;
    };
};
