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
    }
};

function SpatialComputer() {
    this.devices = new Array();
    this.time = 0.0;
    this.init = function() {
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
       }
    };

    this.update = function() {
       for(mid=0; mid < simulatorSettings.numDevices; mid++) {

          // while (!machine.finished()) machine.step();
          this.devices[mid].machine.executeRound(this.time);

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

          // reset the position differential
          this.devices[mid].machine.dx = 0;
          this.devices[mid].machine.dy = 0;
          this.devices[mid].machine.dz = 0;

       }

       this.time = this.time + simulatorSettings.stepSize;
    };
};
