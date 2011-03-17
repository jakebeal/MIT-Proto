#include "odebodyfactory.h"
//#include "xerces_bodies.h"

ODEBodyFactory::ODEBodyFactory(const char* body_file) {
	bodySize = 1.0;
	defaultSize = 10.0;
	x = 0; // -5
	y = 0.0;
	z = 5.5;
	space = 10.0;
	count = 0;

	cout << "Creating parser..." << endl;
	parser = new XmlWorldParser();
	parser->process_xml(body_file);
	cout << "Parser created." << endl;
}

const char* ODEBodyFactory::getBodyFile() {
	return body_file;
}

void ODEBodyFactory::setBodyFile(const char* file) {
	body_file = file;
}

void ODEBodyFactory::create_joints() {
	cout << "Creating joints" << endl;
	while (!parser->jointList.empty()) {
		XmlJoint* joint = parser->jointList.back();

		map<string, ODEBody*>::iterator iter = bodyMap.begin();

		dBodyID bod1;
		dBodyID bod2;
		if ( joint->id1.compare("world") == 0) {
			bod1 = 0;
		} else {
			iter = bodyMap.find(joint->id1);
			if (iter == bodyMap.end()) {
				throw "missing body";
			} else {
				bod1 = iter->second->body;
			}
		}

		if ( joint->id2.compare("world") == 0) {
			bod2 = 0;
		} else {
			iter = bodyMap.find(joint->id2);
			if (iter == bodyMap.end()) {
				throw "missing body";
			} else {
				bod2 = iter->second->body;
			}
		}

		joint->createJoint(world, bod1, bod2);

		parser->jointList.pop_back();
	}
}

bool ODEBodyFactory::empty() {
	return parser->bodyList.empty();
}

int ODEBodyFactory::numBodies() {
	return parser->bodyList.size();
}

ODEBody* ODEBodyFactory::next_body(ODEDynamics* parent, Device* d) {

	XmlBody* nextBody = parser->bodyList.back();
	if (nextBody == NULL) {
		cout << "BODY IS NULL" << endl;
	}
	//	cout<<nextBody.mass<<endl;
	ODEBody* b = ((XmlBox*) nextBody)->getODEBody(parent, d);
	pair<map<string, ODEBody*>::iterator, bool> ret = bodyMap.insert(pair<
			string, ODEBody*> (nextBody->id, b));
	parser->bodyList.pop_back();
	delete nextBody;

	if (parser->bodyList.empty()) {
		this->create_joints();
	}

	if (ret.second) {
		return b;
	} else {
		throw "duplicate body id";
	}

}
