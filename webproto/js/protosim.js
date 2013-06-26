
// Timing
var paused = false;
var time = 0.0;

// set the scene size
var WIDTH = $('#container').width(),
  HEIGHT = $('#container').height();

// set some camera attributes
var VIEW_ANGLE = 45,
  ASPECT = WIDTH / HEIGHT,
  NEAR = 0.1,
  FAR = 10000;

var renderer = new THREE.WebGLRenderer();
var scene = new THREE.Scene();
var camera =
  new THREE.PerspectiveCamera(
    VIEW_ANGLE,
    ASPECT,
    NEAR,
    FAR);

var stadiumXSize = 100;
var stadiumYSize = 100;
var stadiumZSize = 100;

var numSpheres = 100;
var spheres = new Array();
var stats = new Stats();

function keyHandlers(e) {
   if(e.keyCode == 32) { //(space)
      if(paused) {
         paused = false;
         $.jnotify("Un-Paused");
      } else {
         paused = true;
         $.jnotify("Paused");
      }
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
   camera.position.z = 300;

   // start the renderer
   renderer.setSize($('#container').width(), $('#container').height());

   // attach the render-supplied DOM element
   container.appendChild(renderer.domElement);

   // set up the sphere vars
   var radius = 2,
       segments = 16,
       rings = 16;

   var sphereMaterial =
      new THREE.MeshBasicMaterial(
                                  {
                                     color: 0xCC0000
                                  });


   for(mid = 0; mid < numSpheres; mid++) {

      spheres[mid] = new THREE.Mesh(

                                    new THREE.CubeGeometry(1,1,1),
                                    //radius,
                                    //segments,
                                    //rings),

         sphereMaterial);

      var xpos = (Math.random() * stadiumXSize);
      var ypos = (Math.random() * stadiumYSize);
      var zpos = (Math.random() * stadiumZSize);

      // set it's initial position
      spheres[mid].position = { 
         x: xpos,
         y: ypos, 
         z: zpos
      };

      // add the sphere to the scene
      scene.add(spheres[mid]);

      spheres[mid].machine = new Module.Machine(xpos, ypos, zpos, script);

   }


   // create a point light
   var pointLight =
      new THREE.PointLight(0xFFFFFF);

   // set its position
   pointLight.position.x = 10;
   pointLight.position.y = 50;
   pointLight.position.z = 130;

   // add to the scene
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

   if(!paused) {

      for(mid=0; mid < numSpheres; mid++) {
         spheres[mid].machine.executeRound(time);
         var updatedXpos = spheres[mid].machine.x + spheres[mid].machine.dx;
         var updatedYpos = spheres[mid].machine.y + spheres[mid].machine.dy;
         var updatedZpos = spheres[mid].machine.z + spheres[mid].machine.dz;

         spheres[mid].machine.dx = 0;
         spheres[mid].machine.dy = 0;
         spheres[mid].machine.dz = 0;

         spheres[mid].position = {
            x: updatedXpos,
            y: updatedYpos,
            z: updatedZpos
         };

         spheres[mid].machine.x = updatedXpos;
         spheres[mid].machine.y = updatedYpos;
         spheres[mid].machine.Z = updatedZpos;
      }
      time++;
      
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

