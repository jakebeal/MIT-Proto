/*
 * BodyHandler.cpp
 *
 *  Created on: Jan 25, 2011
 *      Author: jclevela
 */


#include "BodyHandler.h"
#include <iostream>

using namespace std;

BodyHandler::BodyHandler()
{
}

void BodyHandler::startElement(const XMLCh* const name,
                           AttributeList& attributes)
{
    char* message = XMLString::transcode(name);
    cout << "I saw element: "<< message << endl;
    XMLString::release(&message);
}

void BodyHandler::fatalError(const SAXParseException& exception)
{
    char* message = XMLString::transcode(exception.getMessage());
    cout << "Fatal Error: " << message
         << " at line: " << exception.getLineNumber()
         << endl;
    XMLString::release(&message);
}
