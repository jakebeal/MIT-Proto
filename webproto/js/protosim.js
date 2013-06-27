// Settings
var viewSettings = {
    cameraView : {
        angle : 45,
        aspect : (4 / 3),
        near : 0.1,
        far : 10000,
        zposition : 300
    },
    pointLight : new THREE.PointLight(0xFFFFFF),
    pointLightPos : { x:10, y:50, z:130 },
    showWebGlStats : true
};

// Timing
var paused = simulatorSettings.startPaused;

var spatialComputer = new SpatialComputer();

var renderer = new THREE.WebGLRenderer();
var scene = new THREE.Scene();
var camera = new THREE.PerspectiveCamera(
    viewSettings.cameraView.angle,
    viewSettings.cameraView.aspect,
    viewSettings.cameraView.near,
    viewSettings.cameraView.far);

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
   camera.position.z = viewSettings.cameraView.zposition;

   // start the renderer
   renderer.setSize($('#container').width(), $('#container').height());

   // attach the render-supplied DOM element
   container.appendChild(renderer.domElement);

   spatialComputer.init();

   // create a point light
   var pointLight = viewSettings.pointLight;
   pointLight.position = viewSettings.pointLightPos;
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

    if(!paused && simulatorSettings.stopWhen && simulatorSettings.stopWhen(spatialComputer.time)) {
	pause()
    }
    if(!paused) {
	spatialComputer.update();
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
