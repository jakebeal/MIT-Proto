//g++ -g -Wall -pedantic -lxerces-c xerces_bodies.cpp -DMAIN_TEST -o parser
#include "xerces_bodies.h"

using namespace std;
using namespace xercesc;

#define POS_TAG "position"
#define ID_TAG "id"
#define QUAT_TAG "quaternion"
#define MASS_TAG "mass"

#define DIM_TAG "dim"

#define ANCHOR_TAG "anchor"
#define AXIS_TAG "axis"
#define HI_STOP_TAG "hiStop"
#define LOW_STOP_TAG "loStop"

/********************************************************
 ********************  Joints ***************************
 ********************************************************/

/***************************************************
 * XmlJoint implementation
 ***************************************************/
XmlJoint::XmlJoint(DOMElement* element) {
	DOMNodeList* id1List = element->getElementsByTagName(XMLString::transcode(
			"a"));
	if (id1List->getLength() != 1) {
		cout << "no a" << endl;
		throw "no a";
	}
	id1 = string(XMLString::transcode(
			id1List->item(0)->getFirstChild()->getNodeValue()));

	DOMNodeList* id2List = element->getElementsByTagName(XMLString::transcode(
			"b"));
	if (id2List->getLength() != 1) {
		cout << "no b" << endl;
		throw "no b";
	}
	id2 = string(XMLString::transcode(
			id2List->item(0)->getFirstChild()->getNodeValue()));
}

void XmlJoint::createJoint(dWorldID world, ODEBody *bod1, ODEBody *bod2) {
	cout << "Unsupported argument" << endl;
}

/***************************************************
 * XmlFixedJoint implementation
 ***************************************************/
XmlFixedJoint::XmlFixedJoint(DOMElement* element) :
	XmlJoint(element) {
}

void XmlFixedJoint::createJoint(dWorldID world, ODEBody *bod1, ODEBody *bod2) {
	dJointID joint = dJointCreateFixed(world, 0);
	dJointAttach(joint, bod1->body, bod2->body);
	dJointSetFixed(joint);
}

/***************************************************
 * XmlHingedJoint implementation
 ***************************************************/
XmlHingedJoint::XmlHingedJoint(XERCES_CPP_NAMESPACE::DOMElement* element,
		const double* transform) :
	XmlJoint(element) {
	anchor = new double[3];
	axis = new double[3];

	//extract anchor position
	DOMNodeList* anchorList = element->getElementsByTagName(
			XMLString::transcode(ANCHOR_TAG));
	if (anchorList->getLength() != 1) {
		throw exception();
	}
	DOMNode* anchorAtt[3];
	anchorAtt[0] = anchorList->item(0)->getAttributes()->getNamedItem(
			XMLString::transcode("x"));
	anchorAtt[1] = anchorList->item(0)->getAttributes()->getNamedItem(
			XMLString::transcode("y"));
	anchorAtt[2] = anchorList->item(0)->getAttributes()->getNamedItem(
			XMLString::transcode("z"));
	for (int i = 0; i < 3; i++) {
		anchor[i] = atof(XMLString::transcode(anchorAtt[i]->getNodeValue()));
	}

	//extract position
	DOMNodeList* axisList = element->getElementsByTagName(XMLString::transcode(
			AXIS_TAG));
	if (axisList->getLength() != 1) {
		throw exception();
	}
	DOMNode* axisAtt[3];
	axisAtt[0] = axisList->item(0)->getAttributes()->getNamedItem(
			XMLString::transcode("x"));
	axisAtt[1] = axisList->item(0)->getAttributes()->getNamedItem(
			XMLString::transcode("y"));
	axisAtt[2] = axisList->item(0)->getAttributes()->getNamedItem(
			XMLString::transcode("z"));
	for (int i = 0; i < 3; i++) {
		axis[i] = atof(XMLString::transcode(axisAtt[i]->getNodeValue()));
	}

	//extract high stop
	DOMNodeList* hiStopList = element->getElementsByTagName(
			XMLString::transcode(HI_STOP_TAG));
	if (hiStopList->getLength() == 0) {
		hiStop = -dInfinity;
	} else if (hiStopList->getLength() == 1) {
		hiStop = atof(XMLString::transcode(
				hiStopList->item(0)->getFirstChild()->getNodeValue()));
	} else {
		throw "bad high stop";
	}

	//extract low stop
	DOMNodeList* lowStopList = element->getElementsByTagName(
			XMLString::transcode(LOW_STOP_TAG));
	if (lowStopList->getLength() == 0) {
		loStop = dInfinity;
	} else if (lowStopList->getLength() == 1) {
		loStop = atof(XMLString::transcode(
				lowStopList->item(0)->getFirstChild()->getNodeValue()));
	} else {
		throw "bad low stop";
	}

	/**
	 * Apply Transform
	 */
	anchor[0] += transform[0];
	anchor[1] += transform[1];
	anchor[2] += transform[2];

}
void XmlHingedJoint::createJoint(dWorldID world, ODEBody *bod1, ODEBody *bod2) {
	dJointID joint = dJointCreateHinge(world, 0);
	dJointAttach(joint, bod1->body, bod2->body);
	dJointSetHingeAnchor(joint, anchor[0], anchor[1], anchor[2]);
	dJointSetHingeAxis(joint, axis[0], axis[1], axis[2]);
	dJointSetHingeParam(joint, dParamLoStop, loStop);
	dJointSetHingeParam(joint, dParamHiStop, hiStop);
	cout << "Created Hinged joint " << endl;
}


/********************************************************
 ********************  Bodies ***************************
 ********************************************************/

/***************************************************
 * XmlBody implementation
 ***************************************************/
XmlBody::XmlBody(DOMElement* element, const double* transform) {
	pos = new double[3];
	quaternion = new double[4];

	//Extract id
	DOMNodeList* idList = element->getElementsByTagName(XMLString::transcode(
			ID_TAG));
	if (idList->getLength() != 1) {
		throw exception();
	}
	id = string(XMLString::transcode(
			idList->item(0)->getFirstChild()->getNodeValue()));

	//extract position
	DOMNodeList* posList = element->getElementsByTagName(XMLString::transcode(
			POS_TAG));
	if (posList->getLength() != 1) {
		throw exception();
	}
	DOMNode* posAtt[3];
	posAtt[0] = posList->item(0)->getAttributes()->getNamedItem(
			XMLString::transcode("x"));
	posAtt[1] = posList->item(0)->getAttributes()->getNamedItem(
			XMLString::transcode("y"));
	posAtt[2] = posList->item(0)->getAttributes()->getNamedItem(
			XMLString::transcode("z"));
	for (int i = 0; i < 3; i++) {
		pos[i] = atof(XMLString::transcode(posAtt[i]->getNodeValue()));
	}

	//extract orientation
	DOMNodeList* quatList = element->getElementsByTagName(XMLString::transcode(
			QUAT_TAG));
	if (quatList->getLength() != 0) {
		if (quatList->getLength() != 1) {
			throw exception();
		}
		DOMNode* quatAtt[4];
		quatAtt[0] = quatList->item(0)->getAttributes()->getNamedItem(
				XMLString::transcode("q1"));
		quatAtt[1] = quatList->item(0)->getAttributes()->getNamedItem(
				XMLString::transcode("q2"));
		quatAtt[2] = quatList->item(0)->getAttributes()->getNamedItem(
				XMLString::transcode("q3"));
		quatAtt[3] = quatList->item(0)->getAttributes()->getNamedItem(
				XMLString::transcode("q4"));
		for (int i = 0; i < 4; i++) {
			quaternion[i] = atof(XMLString::transcode(
					quatAtt[i]->getNodeValue()));
		}
	}

	//extract mass
	DOMNodeList* massList = element->getElementsByTagName(XMLString::transcode(
			MASS_TAG));
	if (massList->getLength() != 1) {
		throw exception();
	}
	//TODO fix mass assignment
	mass = atof(XMLString::transcode(
			idList->item(0)->getFirstChild()->getNodeValue()));

	/**
	 * Apply transform
	 */
	pos[0] += transform[0];
	pos[1] += transform[1];
	pos[2] += transform[2];

}

ODEBody* XmlBody::getODEBody(ODEDynamics* parent, Device* d) {
	cout << "unimplemented method" << endl;
}


/***************************************************
 * XmlBox implementation
 ***************************************************/
XmlBox::XmlBox(DOMElement* element, const double* transform) :
	XmlBody(element, transform) {
	dim = new double[3];

	//extract position
	DOMNodeList* dimList = element->getElementsByTagName(XMLString::transcode(
			DIM_TAG));
	if (dimList->getLength() != 1) {
		throw exception();
	}
	DOMNode* dimAtt[3];
	dimAtt[0] = dimList->item(0)->getAttributes()->getNamedItem(
			XMLString::transcode("w"));
	dimAtt[1] = dimList->item(0)->getAttributes()->getNamedItem(
			XMLString::transcode("b"));
	dimAtt[2] = dimList->item(0)->getAttributes()->getNamedItem(
			XMLString::transcode("h"));
	for (int i = 0; i < 3; i++) {
		dim[i] = atof(XMLString::transcode(dimAtt[i]->getNodeValue()));
	}
}

ODEBody* XmlBox::getODEBody(ODEDynamics* parent, Device* d) {
	cout << pos[0] << endl;
	cout << pos[1] << endl;
	cout << pos[2] << endl;
	cout << dim[0] << endl;
	cout << dim[1] << endl;
	cout << dim[2] << endl;
	cout << mass << endl;

	return new ODEBox(parent, d, pos[0], pos[1], pos[2], dim[0], dim[1],
			dim[2], mass);
}

/***************************************************
 * XmlSphere implementation
 ***************************************************/
XmlSphere::XmlSphere(DOMElement* element, const double* transform) :
	XmlBody(element, transform) {
	//extract radius
	DOMNodeList* radiusList = element->getElementsByTagName(
			XMLString::transcode("radius"));
	if (radiusList->getLength() != 1) {
		throw exception();
	}
	radius = atof(XMLString::transcode(
			radiusList->item(0)->getFirstChild()->getNodeValue()));
}

ODEBody* XmlSphere::getODEBody(ODEDynamics* parent, Device* d) {
	return new ODESphere(parent, d, pos, quaternion, radius, mass);
}


/***************************************************
 * XmlCylinder implementation
 ***************************************************/
XmlCylinder::XmlCylinder(DOMElement* element, const double* transform) :
	XmlBody(element, transform) {
	//extract radius
	DOMNodeList* radiusList = element->getElementsByTagName(
			XMLString::transcode("radius"));
	if (radiusList->getLength() != 1) {
		throw exception();
	}
	radius = atof(XMLString::transcode(
			radiusList->item(0)->getFirstChild()->getNodeValue()));

	//extract height
	DOMNodeList* heightList = element->getElementsByTagName(
			XMLString::transcode("height"));
	if (heightList->getLength() != 1) {
		throw exception();
	}
	height = atof(XMLString::transcode(
			heightList->item(0)->getFirstChild()->getNodeValue()));

}

ODEBody* XmlCylinder::getODEBody(ODEDynamics* parent, Device* d) {
	return new ODECylinder(parent, d, pos, quaternion, radius, height, mass);
}

/********************************************************
 ********************  Parser ***************************
 ********************************************************/

/*****************************************************
 * XmlWorldParser implementation
 ***************************************************/
void XmlWorldParser::processNode(DOMTreeWalker* tw, const double* pos) {
	cout << "Name:"
			<< XMLString::transcode(tw->getCurrentNode()->getNodeName())
			<< endl;

	do {

		DOMElement* node = (DOMElement*) tw->getCurrentNode();

		/* Process body */
		if (XMLString::equals(XMLString::transcode("body"), node->getNodeName())){
			addBody(node, pos);
		}else
		 /* Process joint */
		if (XMLString::equals(XMLString::transcode("joint"),
				node->getNodeName())){
			addJoint(node, pos);
		}else
		/* Process a transform */
		if (XMLString::equals(XMLString::transcode("transform"),
				node->getNodeName())) {
			double trans[3];
			char* x_c = XMLString::transcode(
					((DOMElement*) tw->getCurrentNode())->getAttribute(
							XMLString::transcode("x")));
			char* y_c = XMLString::transcode(
					((DOMElement*) tw->getCurrentNode())->getAttribute(
							XMLString::transcode("y")));
			char* z_c = XMLString::transcode(
					((DOMElement*) tw->getCurrentNode())->getAttribute(
							XMLString::transcode("z")));

			trans[0] = pos[0] + atof(x_c);
			trans[1] = pos[1] + atof(y_c);
			trans[2] = pos[2] + atof(z_c);

			if (tw->firstChild() != NULL) {
				processNode(tw, trans);
			}
		}else if (tw->firstChild() != NULL) {
				processNode(tw, pos);
		}

	} while (tw->nextSibling() != NULL);

	tw->parentNode();

	return;
}


void XmlWorldParser::addJoint(DOMElement* node, const double* pos) {
	string jointType = string(XMLString::transcode(node->getAttribute(
			XMLString::transcode("type"))));
	XmlJoint* joint;
	try {
		if (jointType == "fixed") {
			joint = new XmlFixedJoint((DOMElement*) node);
		} else if (jointType == "hinged") {
			joint = new XmlHingedJoint((DOMElement*) node, pos);
		} else {
			cout << "Unsupported joint type." << endl;
			throw "Unsupported joint type.";
		}
		jointList.push_back(joint);
		cout<<"Joint: "<<joint->id1<<" + "<<joint->id2<<endl;
	} catch (exception& e) {
		cout << "Poorly formed XML joint" << endl;
	}
}

void XmlWorldParser::addBody(DOMElement* node, const double* pos) {
	try {
		string bodyType = string(XMLString::transcode(node->getAttribute(
				XMLString::transcode("type"))));
		XmlBody* xBody;

		if (bodyType == "box") {
			xBody = new XmlBox(node, pos);
		} else if (bodyType == "sphere") {
			xBody = new XmlSphere(node, pos);
		} else if (bodyType == "cylinder") {
			xBody = new XmlCylinder(node, pos);
		} else {
			cout << "Unknown body type." << endl;
			throw "Unknown body type.";
		}
		bodyList.push_back(xBody);
		cout<<"Body ID: "<<xBody->id<<endl;
	} catch (exception& e) {
		cout << "Poorly formed XML body " << endl;
	}
}

bool XmlWorldParser::process_xml(const char* xmlFile) {

	try {
		XMLPlatformUtils::Initialize();
	} catch (const XMLException& toCatch) {
		char* message = XMLString::transcode(toCatch.getMessage());
		cout << "Error during initialization! :\n" << message << "\n";
		XMLString::release(&message);
		return 1;
	}

	XercesDOMParser* parser = new XercesDOMParser();
	parser->setValidationScheme(XercesDOMParser::Val_Always);
	parser->setDoNamespaces(true); // optional

	/**
	 * Configure parser error handling
	 */
	ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
	parser->setErrorHandler(errHandler);
	try {
		parser->parse(xmlFile);
	} catch (const XMLException& toCatch) {
		char* message = XMLString::transcode(toCatch.getMessage());
		cout << "Exception message is: \n" << message << "\n";
		XMLString::release(&message);
		return -1;
	} catch (const DOMException& toCatch) {
		char* message = XMLString::transcode(toCatch.msg);
		cout << "Exception message is: \n" << message << "\n";
		XMLString::release(&message);
		return -1;
	} catch (...) {
		cout << "Unexpected Exception \n";
		return -1;
	}

	/**
	 * Walk through the document, adding bodies and joints in their relative frames
	 */
	DOMDocument* doc = parser->getDocument();
	DOMTreeWalker* walker = doc->createTreeWalker(
										doc->getDocumentElement(),
										DOMNodeFilter::SHOW_ELEMENT,
										new BodiesInWorld(),
										true);
	/** Initial world frame */
	double transform[3] = { 0, 0, 0 };
	processNode(walker, transform);

	//TODO Ensure that I am cleaning up everything I need to
	/** Clean up no longer needed resources **/
	doc->release();
	delete errHandler;

	return true;
}

/***************************************************
 * BodiesInWorld Implementation
 ***************************************************/
DOMNodeFilter::FilterAction BodiesInWorld::acceptNode(const DOMNode* node) const {
	if (node->getNodeType() == 1) {

		string name = string(XMLString::transcode( node->getNodeName()));
		cout<<"NODE NAME:"<<name<<endl;
		if (name.compare("body") == 0)
			return FILTER_ACCEPT;

		if (name.compare("joint") == 0)
			return FILTER_ACCEPT;

		if (name.compare("transform") == 0)
					return FILTER_ACCEPT;

		if (name.compare("root") == 0)
							return FILTER_SKIP;
		if (name.compare("bodies") == 0)
							return FILTER_SKIP;
		if (name.compare("joints") == 0)
							return FILTER_SKIP;

	}

	  return FILTER_REJECT;
}
