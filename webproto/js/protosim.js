// Settings
var settings = {
      numDevices : 100,
      radius : 10,
      stadiumSize : { x:100, y:100, z:100 },
      material : new THREE.MeshBasicMaterial( { color: 0xCC0000 }),
      deviceShape : new THREE.CubeGeometry(1,1,1),
      cameraView : {
         angle : 45,
         aspect : (4 / 3),
         near : 0.1,
         far : 10000,
         zposition : 300
      },
      pointLight : new THREE.PointLight(0xFFFFFF),
      pointLightPos : { x:10, y:50, z:130 },
      stepSize : 1,
      startPaused : false,
      startTime : 0.0,
      showWebGlStats : true,
      distribution : function(mid) {
         return {
            x : (Math.random() * settings.stadiumSize.x), 
            y : (Math.random() * settings.stadiumSize.y), 
            z : (Math.random() * settings.stadiumSize.z)
         };
      },
      stopWhen : function(time) {
         return false;
      },
      positionMultipliers : { x:1, y:1, z:1 }
   };

// Timing
var paused = settings.startPaused;
var time = settings.startTime;

var renderer = new THREE.WebGLRenderer();
var scene = new THREE.Scene();
var camera = new THREE.PerspectiveCamera(
    settings.cameraView.angle,
    settings.cameraView.aspect,
    settings.cameraView.near,
    settings.cameraView.far);

var devices = new Array();
var stats = new Stats();

function unpause() {
   paused = false;
   $.jnotify("Un-Paused");
}

function pause() {
   paused = true;
   $.jnotify("Paused");
}

function togglePause() {
   if(paused) {
      unpause();
   } else {
      pause();
   }
}

function keyHandlers(e) {
   if(e.keyCode == 32) { //(space)
      togglePause();
   }
};

function init() {

   // get the DOM element to attach to
   var $container = $('#container');

   stats.domElement.style.position = 'absolute';
   stats.domElement.style.top = '0px';
   container.appendChild( stats.domElement );

   // add the camera to the scene
   scene.add(camera);

   // the camera starts at 0,0,0
   // so pull it back
   camera.position.z = settings.cameraView.zposition;

   // start the renderer
   renderer.setSize($('#container').width(), $('#container').height());

   // attach the render-supplied DOM element
   container.appendChild(renderer.domElement);

   // initialize the devices
   for(mid = 0; mid < settings.numDevices; mid++) {

      devices[mid] = new THREE.Mesh(settings.deviceShape, settings.material);

      // set it's initial position
      devices[mid].position = settings.distribution(mid);

      // add the sphere to the scene
      scene.add(devices[mid]);

      // initialize the Proto VM
      devices[mid].machine = new Module.Machine(
         devices[mid].position.x, 
         devices[mid].position.y, 
         devices[mid].position.z, 
         script);
   }


   // create a point light
   var pointLight = settings.pointLight;
   pointLight.position = settings.pointLightPos;
   scene.add(pointLight);

   renderer.render(scene, camera);

   controls = new THREE.TrackballControls(camera, renderer.domElement);

   controls.rotateSpeed = 1.0;
   controls.zoomSpeed = 1.2;
   controls.panSpeed = 0.8;

   controls.noZoom = false;
   controls.noPan = false;

   controls.staticMoving = true;
   controls.dynamicDampingFactor = 0.3;

   controls.keys = [65, 83, 68];

   camera.lookAt(scene.position);
};

function animate() {

   if(!paused && settings.stopWhen && settings.stopWhen(time)) {
      pause()
   }

   if(!paused) {

      for(mid=0; mid < settings.numDevices; mid++) {

         // while (!machine.finished()) machine.step();
         devices[mid].machine.executeRound(time);

         // update the position of the device
         devices[mid].position = {
            x: devices[mid].machine.x + (devices[mid].machine.dx * settings.positionMultipliers.x),
            y: devices[mid].machine.y + (devices[mid].machine.dy * settings.positionMultipliers.y),
            z: devices[mid].machine.z + (devices[mid].machine.dz * settings.positionMultipliers.z)
         };

         // update the position of the Proto VM
         devices[mid].machine.x = devices[mid].position.x;
         devices[mid].machine.y = devices[mid].position.y;
         devices[mid].machine.z = devices[mid].position.z;

         // reset the position differential
         devices[mid].machine.dx = 0;
         devices[mid].machine.dy = 0;
         devices[mid].machine.dz = 0;

      }

      time = time + settings.stepSize;
      
  }

  requestAnimationFrame( animate );
  controls.update();
  render();
  
  stats.update();

};

function render() {
  renderer.setSize($('#container').width(), $('#container').height());
  renderer.render( scene, camera );
};

