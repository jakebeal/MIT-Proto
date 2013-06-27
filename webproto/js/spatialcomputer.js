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

var spatialComputer = {
    devices : new Array(),
    time : 0.0
};

spatialComputer.init = function() {
    spatialComputer.time = simulatorSettings.startTime;
    
    // initialize the devices
    for(mid = 0; mid < simulatorSettings.numDevices; mid++) {
	
	spatialComputer.devices[mid] = new THREE.Mesh(simulatorSettings.deviceShape, simulatorSettings.material);
	
	// set it's initial position
	spatialComputer.devices[mid].position = simulatorSettings.distribution(mid);
	
	// add the sphere to the scene
	scene.add(spatialComputer.devices[mid]);
	
	// initialize the Proto VM
	spatialComputer.devices[mid].machine = new Module.Machine(
	    spatialComputer.devices[mid].position.x, 
	    spatialComputer.devices[mid].position.y, 
	    spatialComputer.devices[mid].position.z, 
	    script);
    }
}
    
spatialComputer.update = function() {
    for(mid=0; mid < simulatorSettings.numDevices; mid++) {
	
        // while (!machine.finished()) machine.step();
        spatialComputer.devices[mid].machine.executeRound(spatialComputer.time);
	
        // update the position of the device
        spatialComputer.devices[mid].position = {
            x: spatialComputer.devices[mid].machine.x + (spatialComputer.devices[mid].machine.dx),
            y: spatialComputer.devices[mid].machine.y + (spatialComputer.devices[mid].machine.dy),
            z: spatialComputer.devices[mid].machine.z + (spatialComputer.devices[mid].machine.dz)
        };
	
        // update the position of the Proto VM
        spatialComputer.devices[mid].machine.x = spatialComputer.devices[mid].position.x;
        spatialComputer.devices[mid].machine.y = spatialComputer.devices[mid].position.y;
        spatialComputer.devices[mid].machine.z = spatialComputer.devices[mid].position.z;
	
        // reset the position differential
        spatialComputer.devices[mid].machine.dx = 0;
        spatialComputer.devices[mid].machine.dy = 0;
        spatialComputer.devices[mid].machine.dz = 0;
	
    }
    
    spatialComputer.time = spatialComputer.time + simulatorSettings.stepSize;
}


