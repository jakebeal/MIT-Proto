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

   document.addEventListener( 'mousemove', function(event) {
      event.preventDefault();
      mouse = {
         x : ( event.clientX / window.innerWidth ) * 2 - 1,
         y : - ( event.clientY / window.innerHeight ) * 2 + 1
      };
   }, false );

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
    
    requestAnimationFrame( animate );
    controls.update();
    render();
    
    stats.update();
};

var INTERSECTED;

function onMouseOverObject(object) {
}

function onMouseOffObject(object) {
}

function render() {
  renderer.setSize($('#container').width(), $('#container').height());

  // find intersections
  if(mouse && mouse.x && mouse.y) {
     var vector = new THREE.Vector3( mouse.x, mouse.y, 1 );
     projector.unprojectVector( vector, camera );
     raycaster.set( camera.position, vector.sub( camera.position ).normalize() );
     var intersects = raycaster.intersectObjects( scene.children );
     if ( intersects.length > 0 ) {
        if ( INTERSECTED != intersects[ 0 ].object ) {
           if ( INTERSECTED ) {
              //INTERSECTED.material.emissive.setHex( INTERSECTED.currentHex );
              onMouseOffObject(INTERSECTED);
           }
           INTERSECTED = intersects[ 0 ].object;
           onMouseOverObject(INTERSECTED)
           //INTERSECTED.currentHex = INTERSECTED.material.emissive.getHex();
           //INTERSECTED.material.emissive.setHex( 0xff0000 );
        }
     } else {
        //if ( INTERSECTED ) INTERSECTED.material.emissive.setHex( INTERSECTED.currentHex );
        if( INTERSECTED ) onMouseOffObject(INTERSECTED);
        INTERSECTED = null;
     }
  }


  renderer.render( scene, camera );
};
