	/*
 * BodyHandler.h
 *
 *  Created on: Jan 25, 2011
 *      Author: jclevela
 */

#ifndef BODYHANDLER_H_
#define BODYHANDLER_H_
#include <xercesc/sax/HandlerBase.hpp>

class BodyHandler : public HandlerBase {
public:
    void startElement(const XMLCh* const, AttributeList&);
    void fatalError(const SAXParseException&);
};

#endif /* BODYHANDLER_H_ */
