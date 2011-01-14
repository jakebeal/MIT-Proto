
#ifndef CPP_UNIT_TYPECHECKINGTESTCASE_H
#define CPP_UNIT_TYPECHECKINGTESTCASE_H

#include <cppunit/extensions/HelperMacros.h>

/**
 * A test case for Proto TypeChecking.
 */
class TypeCheckingTestCase : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( TypeCheckingTestCase );
  CPPUNIT_TEST( example );
  CPPUNIT_TEST( deliteralizeScalar );
  CPPUNIT_TEST( deliteralizeTuple );
  CPPUNIT_TEST( getNthArg );
  CPPUNIT_TEST( getRefSymbol );
  CPPUNIT_TEST( getRefUnlit );
  CPPUNIT_TEST( getRefFieldof );
  CPPUNIT_TEST( getRefFt );
  CPPUNIT_TEST( getRefInputs );
  CPPUNIT_TEST( getRefLast );
  CPPUNIT_TEST( getRefLcs );
  CPPUNIT_TEST( getRefNth );
  CPPUNIT_TEST( getRefOutput );
  CPPUNIT_TEST( getRefTupof );
  CPPUNIT_TEST( getRefOptionalInputs );
  CPPUNIT_TEST( getRefRest );
  CPPUNIT_TEST_SUITE_END();

protected:

public:
  void setUp();

protected:
  void example();
  void deliteralizeScalar();
  void deliteralizeTuple();
  void getNthArg();
  void getRefSymbol();
  void getRefUnlit();
  void getRefFieldof();
  void getRefFt();
  void getRefInputs();
  void getRefLast();
  void getRefLcs();
  void getRefNth();
  void getRefOutput();
  void getRefTupof();
  void getRefOptionalInputs();
  void getRefRest();
};


#endif
