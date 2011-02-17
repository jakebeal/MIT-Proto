#include "odebodyfactory.h"
//#include "xerces_bodies.h"

ODEBodyFactory::ODEBodyFactory(const char* body_file){
	 bodySize = 1.0;
	 defaultSize = 10.0;
	 x = 0; // -5
	 y = 0.0;
	 z = 5.5;
	 space = 10.0;
	 count = 0;


	 cout<<"Creating parser..."<<endl;
	 parser = new XmlWorldParser();
//	 parser->process_xml("test.xml");
//	 printf("body file is:%s\n",body_file);
	 parser->process_xml(body_file);

	 cout<<"Parser created."<<endl;
}

//static ODEBodyFactory::ODEBodyFactory* instanceof(){
//	if( ODEBodyFactory::instance == NULL){
//		ODEBodyFactory::instance = new ODEBodyFactory();
//	}
//
//	return ODEBodyFactory::instance;
//}

const char* ODEBodyFactory::getBodyFile(){
	return body_file;
}

void ODEBodyFactory::setBodyFile(const char* file){
	body_file = file;
}

void ODEBodyFactory::create_joints(){
	cout<<"Creating joints"<<endl;
	while(!parser->jointList.empty()){
		XmlJoint* joint = parser->jointList.back();

		map<string, ODEBody*>::iterator iter = bodyMap.begin();

		ODEBody* bod1;
		ODEBody* bod2;

		iter = bodyMap.find(joint->id1);
		if(iter == bodyMap.end() ){
			throw "missing body";
		}else{
			bod1 = iter->second;
		}

		iter = bodyMap.find(joint->id2);
		if(iter == bodyMap.end() ){
			throw "missing body";
		}else{
			bod2 = iter->second;
		}
		cout<<"Joint 1:"<<joint->id1<<", joint 2:"<<joint->id2<<endl;
		joint->createJoint(world, bod1, bod2);

		parser->jointList.pop_back();
	}
}

bool ODEBodyFactory::empty(){
	return parser->bodyList.empty();
}

int ODEBodyFactory::numBodies(){
	return parser->bodyList.size();
}

ODEBody* ODEBodyFactory::next_body(ODEDynamics* parent, Device* d){

	XmlBody* nextBody = parser->bodyList.back();
	cout<<endl<<"about to get body"<<endl;
	if(nextBody == NULL){
		cout<<"BODY IS NULL"<<endl;
	}
//	cout<<nextBody.mass<<endl;
	cout<<"List size:"<<parser->bodyList.size()<<endl;
	ODEBody* b = ((XmlBox*)nextBody)->getODEBody(parent, d);
	pair< map<string, ODEBody*>::iterator, bool> ret = bodyMap.insert( pair<string, ODEBody*>(nextBody->id, b));
	parser->bodyList.pop_back();
	delete nextBody;

	if( parser->bodyList.empty() ){
		this->create_joints();
	}

	if( ret.second ){
		return b;
	}else{
		throw "duplicate body id";
	}

}


ODEBody* ODEBodyFactory::next_body2(ODEDynamics* parent, Device* d){
 return NULL;
/*
	  f_position = new flo[3];
	  f_orientation = new flo[4];
	  f_velocity = new flo[3];
	  f_ang_velocity = new flo[3];

	  this->parent=parent; moved=FALSE;
	  for(int i=0;i<3;i++) desired_v[i]=0;
	  did_bump=false;
	  // create and attach body, shape, and mass
	  body = dBodyCreate(parent->world);
	  geom = dCreateBox(parent->space,r*2,r*2,r*2);
	  dMass m;

	  dMassSetBox(&m,parent->density,r*2,r*2,r*2);

	  dGeomSetBody(geom,body);


	  dBodySetMass(body,&m);
	  // set position and orientation
	  dBodySetPosition(body, x, y, z);
	  dQuaternion Q; dQFromAxisAndAngle (Q,0,0,1,0);
	  dBodySetQuaternion (body,Q);
	  // set up back-pointer
	  dBodySetData(body,(void*)this);
	  dGeomSetData(geom,(void*)this);

	   if(parent->is_2d){
		  dJointID planeJointID = dJointCreatePlane2D( parent->world, 0);
		  dJointAttach( planeJointID, body, 0 );
	   }
*/

//
//	  post("new body from factory!");
//	   ODEBody* b;
//	   dJointID planeJointID;
//	   dJointID fixedJoint;
//	   switch(count){
//		   case 0:
//			b = new ODEBody(parent, d, 0, 0, 0, 20, 10, 10, 10000);
////			planeJointID = dJointCreatePlane2D( world, 0);
//			fixedJoint = dJointCreateFixed(world, 0);
//			dJointAttach( fixedJoint, b->body, 0 );
//			break;
//		   case 1:
//			   b = new ODEBody(parent, d, 13, 0, 0, 2, 5, 30, 1);
//			   dJointID joint = dJointCreateHinge (world, 0);
//			   dJointAttach (joint, b->body, ((ODEBody*)parent->bodies.get(0))->body );
//			   dJointSetHingeAnchor (joint, 10, 0, 0 );
//			   dJointSetHingeAxis (joint, 1, 0, 0);
//			   dJointSetHingeParam (joint, dParamLoStop,  -dInfinity );
//			   dJointSetHingeParam (joint, dParamHiStop, dInfinity);
//
//			   dJointID motor =  dJointCreateAMotor (world, 0);
//			   dJointSetAMotorMode (motor, dAMotorUser);
//
//			   dJointAttach (motor, b->body, ((ODEBody*)parent->bodies.get(0))->body );
//			   dJointSetAMotorNumAxes (motor, 1);
//			   dJointSetAMotorAxis (motor, 0, 2,1, 0, 0);
//			   dJointSetAMotorParam (motor, dParamFMax, 20);
//
//			   dJointSetAMotorParam (motor, dParamVel, 0.2);
//	   }
//	   if(count == 0 || count == 1){
//	   }else{
//		   b = new ODEBody(parent, d, x, y, 0.0, bodySize);
//	   }
//
//	   if(count == 3){
//		   dJointID joint = dJointCreateFixed(world, 0);
//
//		   dJointAttach(joint, b->body, ((ODEBody*)parent->bodies.get(2))->body);
//		   dJointSetFixed(joint);
//	   }
//
//	   if(count == 5){
//		   //add joint
////		   dJointID joint = dJointCreateSlider(world, 0);
//
//
//
//		   dJointID joint = dJointCreateHinge (world, 0);
//		   dJointAttach (joint, b->body, ((ODEBody*)parent->bodies.get(4))->body );
//		   dJointSetHingeAnchor (joint, x, y, 0 );
//		   dJointSetHingeAxis (joint, 0, 0, 1);
//		   dJointSetHingeParam (joint, dParamLoStop, -1.57);
//		   dJointSetHingeParam (joint, dParamHiStop, 1.57);
////
////		   dJointID joint = dJointCreateBall(world, 0);
////		   dJointSetBallAnchor (joint, x - bodySize, 0,0);
//
//	   }
//
//	x += 2*bodySize + space;
//	bodySize += 1.0;
//	count++;
//	return b;
}
