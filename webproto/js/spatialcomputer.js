var simulatorSettings = {
    numDevices : 100,
    radius : 35,
    drawEdges : false,
    stadiumRegion : { x_min:-50, x_max:50, 
		      y_min:-50, y_max:50, 
		      z_min:0, z_max:0 },
    distributionRegion : false, // default to stadiumsize
    lineMaterial : new THREE.LineBasicMaterial( { color: 0x00CC00, opacity: 0.8, linewidth: 0.5 } ),
    material : function(mid) {
      return new THREE.MeshBasicMaterial( { color: 0xCC0000 });
    },
    deviceShape : function(mid) {
      return new THREE.CubeGeometry(1,1,1);
    },
    stepSize : 1.0,
    startPaused : false,
    startTime : 0.0,
    distribution : function(mid) {
       if (simulatorSettings.distributionRegion) { 
	   size = simulatorSettings.distributionRegion;
       } else { 
	   size = simulatorSettings.stadiumRegion;
       }
       return {
          x : size.x_min + (Math.random() * (size.x_max - size.x_min)), 
          y : size.y_min + (Math.random() * (size.y_max - size.y_min)), 
          z : size.z_min + (Math.random() * (size.z_max - size.z_min)), 
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

function distanceBetweenDevices(deviceA, deviceB) {
   var ma = deviceA.machine;
   var mb = deviceB.machine;
   return Math.sqrt(Math.pow(ma.x - mb.x, 2) + Math.pow(ma.y - mb.y, 2) + Math.pow(ma.z - mb.z, 2));
};

/**
 * Neighbors from the persepctive of deviceA
 */
function areNeighbors(deviceA, deviceB) {
   return distanceBetweenDevices(deviceA, deviceB) <= deviceA.machine.radius;
}

/**
 * Calls toCallOnNeighbors() on each device in allDevices that
 * are a neighbor of device.
 *
 * toCallOnNeighbors() will be called with two arguments:
 * 1) the neighbor device
 * 2) device (i.e., from args)
 */
function neighborMap(device, allDevices, toCallOnNeighbors, needToUpdateNeighbors) {
   if(needToUpdateNeighbors || !device.neighbors) {
      device.neighbors = new Array();
      $.each(allDevices, function(index, value) {
         if(areNeighbors(device, value)) {
            device.neighbors.push(value);
         }
      });
   }

   $.each(device.neighbors, function(index, value) {
      if(toCallOnNeighbors) {
         toCallOnNeighbors(value, device);
      }
   });
}

function SpatialComputer() {
    this.devices = new Array();
    this.time = 0.0;
    this.nextMid = 0;
    this.getNextMid = function() {
      return this.nextMid++;
    };

    this.addDevice = function() {
       var mid = this.getNextMid();
       this.devices[mid] = new THREE.Mesh(simulatorSettings.deviceShape(mid), simulatorSettings.material(mid));

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

       this.devices[mid].machine.id = mid;
       
       this.devices[mid].machine.radius = simulatorSettings.radius;

       this.devices[mid].machine.resetActuators = function() {
          this.dx = 0;
          this.dy = 0;
          this.dz = 0;
          this.red = 0;
          this.blue = 0;
          this.green = 0;
       }

       // Set the device's internal time
       this.devices[mid].time = this.time;

       // Set the device's model of time
       this.devices[mid].deviceTimer = {
          nextTransmit : function(currentTime) { return 0.5 + currentTime; },
          nextCompute : function(currentTime) { return 1 + currentTime; }
       };

       // Initialize the device's next compute/transmit time
       this.devices[mid].nextTransmitTime = 
          this.devices[mid].deviceTimer.nextTransmit(this.time);
       this.devices[mid].nextComputeTime = 
          this.devices[mid].deviceTimer.nextCompute(this.time);

       // Flags for die/clone
       this.devices[mid].requestDeath = false;
       this.devices[mid].requestClone = false;

       if(simulatorSettings.deviceInitHook) { simulatorSettings.deviceInitHook(this.device[mid]); }
    };

    this.init = function() {
      if(simulatorSettings.preInitHook) { simulatorSettings.preInitHook(); }

       this.time = simulatorSettings.startTime;

       // initialize the devices
       for(mid = 0; mid < simulatorSettings.numDevices; mid++) {
         this.addDevice();
       }
    };
    
    this.updateColors = function() {
       for(mid=0; mid < simulatorSettings.numDevices; mid++) {
          // update the color of the device
          if(this.devices[mid].machine.red > 0 ||
                    this.devices[mid].machine.green > 0 ||
                    this.devices[mid].machine.blue > 0) 
          {
             this.devices[mid].material.color.r = this.devices[mid].machine.red;
             this.devices[mid].material.color.g = this.devices[mid].machine.green;
             this.devices[mid].material.color.b = this.devices[mid].machine.blue;
          } else if(this.devices[mid].machine.getSensor(1)) {
             this.devices[mid].material.color = { r:1.0, g:0.5, b:0 };
          } else if(this.devices[mid].machine.getSensor(2)) {
             this.devices[mid].material.color = { r:0.5, g:0, b:1 };
          } else if(this.devices[mid].machine.getSensor(3)) {
             this.devices[mid].material.color = { r:1, g:0.5, b:1 };
          } else if(this.devices[mid].selected) {
             this.devices[mid].material.color = { r:1, g:1, b:1 };
          } else {
             // Default color (red)
             this.devices[mid].material.color.r = 0.8;
             this.devices[mid].material.color.g = 0.0;
             this.devices[mid].material.color.b = 0.0;
          }

       } // end foreach device
    };

    this.needToUpdateNeighbors = false;

    this.update = function() {
       if(simulatorSettings.preUpdateHook) { simulatorSettings.preUpdateHook(); }

       this.needToUpdateNeighbors = false;
       
       // for each device...
       for(mid=0; mid < simulatorSettings.numDevices; mid++) {
       
          // if it's time to transmit...
          if(this.time >= this.devices[mid].nextTransmitTime) {
             // deliver messages to my neighbors
             neighborMap(this.devices[mid],this.devices,
                         function (nbr,d) { 
                            d.machine.deliverMessage(nbr.machine); 
                         }, this.needToUpdateNeighbors);
          }

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

          } // end if(shouldCompute)
          
          if(this.devices[mid].machine.dx > 0 ||
             this.devices[mid].machine.dy > 0 ||
             this.devices[mid].machine.dz > 0) 
          {
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

             // indicate that we need to update the nieghbors because someone moved
             this.needToUpdateNeighbors = true;
          }

          // clone, TODO: die
          if(this.devices[mid].requestClone) {
             spatialComputer.addDevice();
          }

       } // end foreach device

       // update the simulator time
       this.time = this.time + simulatorSettings.stepSize;

    }; // end update() function

};
