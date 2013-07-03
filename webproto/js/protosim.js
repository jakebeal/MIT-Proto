// Settings
var viewSettings = {
    cameraView : {
        angle : 45,
        aspect : (4 / 3),
        near : 0.1,
        far : 10000
    },
    baseLight : new THREE.AmbientLight(0x202020), // soft white light
    highLight : new THREE.DirectionalLight(0xC0C0C0), // hard white light
    showWebGlStats : true,
    antialias : true
};

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

function destroy() {
   //?
}

var paused, spatialComputer, renderer, scene, camera, stats, projector, raycaster, mouse;
var INTERSECTED;

var neighborLines = new Array();

function init() {
   projector = new THREE.Projector();
   raycaster = new THREE.Raycaster();
   scene = new THREE.Scene();
   renderer = new THREE.WebGLRenderer( { antialias: viewSettings.antialias } );
   camera = new THREE.PerspectiveCamera(
                                            viewSettings.cameraView.angle,
                                            viewSettings.cameraView.aspect,
                                            viewSettings.cameraView.near,
                                            viewSettings.cameraView.far);
   stats = new Stats();
   
   // Timing
   paused = simulatorSettings.startPaused;

   spatialComputer = new SpatialComputer();

   // get the DOM element to attach to
   var $container = $('#container');

   stats.domElement.style.position = 'absolute';
   stats.domElement.style.top = '0px';
   container.appendChild( stats.domElement );

   // add the camera to the scene
   scene.add(camera);

   // the camera starts at 0,0,0
   // so pull it back to see the whole arena
   aspect_ratio = $('#container').width() / $('#container').height();
   y_angle = Math.PI/8; // half of 45 degrees, in radians
   width = simulatorSettings.stadiumRegion.x_max - 
	simulatorSettings.stadiumRegion.x_min;
   height = simulatorSettings.stadiumRegion.y_max -
	simulatorSettings.stadiumRegion.y_min;
   fit_zy = (width/2) / Math.tan(y_angle);
   x_angle = Math.atan(Math.tan(y_angle)*aspect_ratio);
   fit_zx = (height/2) / Math.tan(x_angle);
   camera.position.z = Math.max(fit_zx, fit_zy)*1.05;

   // start the renderer
   renderer.setSize($('#container').width(), $('#container').height());

   // attach the render-supplied DOM element
   container.appendChild(renderer.domElement);

   // create the light sources
   scene.add(viewSettings.baseLight);
   scene.add(viewSettings.highLight);
   viewSettings.highLight.position.set(0.5,0.2,1);

   spatialComputer.init();

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

   document.addEventListener( 'click', function(event) {
      event.preventDefault();
      function onMouseClickObject(object) {
         INTERSECTED.selected = true;
         console.log(INTERSECTED);
      }
      function onMouseUnClickObject(object) {
         INTERSECTED.selected = false;
      }
      mouse = {
         x : ( event.clientX / $('#container').width() ) * 2 - 1,
         y : - ((event.clientY - $('#header').height()) / (window.innerHeight - $('#header').height() - $('#footer').height())  * 2 - 1)
      };
      // find intersections
      if(mouse && mouse.x && mouse.y) {
         var vector = new THREE.Vector3( mouse.x, mouse.y, 1 );
         projector.unprojectVector( vector, camera );
         raycaster.set( camera.position, vector.sub( camera.position ).normalize() );
         var intersects = raycaster.intersectObjects( scene.children );
         if ( intersects.length > 0 ) {
            if ( INTERSECTED != intersects[ 0 ].object ) {
               if ( INTERSECTED ) {
                  onMouseUnClickObject(INTERSECTED);
               }
               INTERSECTED = intersects[ 0 ].object;
               onMouseClickObject(INTERSECTED)
            }
         } else {
            if( INTERSECTED ) onMouseUnClickObject(INTERSECTED);
            INTERSECTED = null;
         }
      }
   }, false );

   document.addEventListener('keypress', function(event) {
      if(INTERSECTED) { // a device is selected
         if(event.which == 116) { // "t" pressed
            INTERSECTED.machine.setSensor(1,
               INTERSECTED.machine.getSensor(1) ? false : true);
         }
         if(event.which == 121) { // "y" pressed
            INTERSECTED.machine.setSensor(2,
               INTERSECTED.machine.getSensor(2) ? false : true);
         }
         if(event.which == 117) { // "u" pressed
            INTERSECTED.machine.setSensor(3,
               INTERSECTED.machine.getSensor(3) ? false : true);
         }
      }
   }, false);

};

function animate() {

    if(!paused && simulatorSettings.stopWhen && simulatorSettings.stopWhen(spatialComputer.time)) {
       pause()
    }
    if(!paused) {
       spatialComputer.update();

       if(neighborLines.length > 0) {
         $.each(neighborLines, function(index, line) {
            scene.remove(line);
         });
         neighborLines = new Array();
       }

       if(simulatorSettings.drawEdges) {
          $.each(spatialComputer.devices, function(index, device) {
             neighborMap(device, spatialComputer.devices, function(neighbor, d) {
                var geom = new THREE.Geometry();
                geom.vertices.push(new THREE.Vector3(device.position.x, device.position.y, device.position.z));
                geom.vertices.push(new THREE.Vector3(neighbor.position.x, neighbor.position.y, neighbor.position.z));
                var line = new THREE.Line(geom, simulatorSettings.lineMaterial, THREE.LinePieces);
                line.scale.x = line.scale.y = line.scale.z = 1;
                line.originalScale = 1;
                line.geometry.__dirtyVerticies = true;
                scene.add(line);
                neighborLines.push(line);
             });
          });
       }
    }

    spatialComputer.updateColors();
    
    requestAnimationFrame( animate );
    controls.update();
    render();
    
    stats.update();
};

function render() {
  renderer.setSize($('#container').width(), $('#container').height());

  renderer.render( scene, camera );
};
