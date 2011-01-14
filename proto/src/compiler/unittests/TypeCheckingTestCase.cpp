#include <cppunit/config/SourcePrefix.h>
#include "TypeCheckingTestCase.h"
//These macros expose the (explicitly) non-public functions 
#define protected public
#define private public
#include "analyzer.h"
#include "ir.h"
#undef protected
#undef private
#include "sexpr.h"

CPPUNIT_TEST_SUITE_REGISTRATION( TypeCheckingTestCase );

void TypeCheckingTestCase::setUp()
{
   //do nothing
}

void TypeCheckingTestCase::example() {
  CPPUNIT_ASSERT( 1 == 1 );
}

void TypeCheckingTestCase::deliteralizeScalar() {
  //deliteralize(<Scalar 2>) = <Scalar>
  ProtoScalar* literal = new ProtoScalar(2);
  ProtoType* delit = Deliteralization::deliteralize(literal);
  CPPUNIT_ASSERT(delit->isA("ProtoScalar"));
  ProtoScalar* delitScalar = S_TYPE(delit);
  CPPUNIT_ASSERT(!delitScalar->isLiteral());

  //deliteralize(deliteralize(<Scalar 2>)) = deliteralize(<Scalar>) = <Scalar>
  ProtoType* redelit = Deliteralization::deliteralize(delit);
  CPPUNIT_ASSERT(redelit->isA("ProtoScalar"));
  ProtoScalar* redelitScalar = S_TYPE(redelit);
  CPPUNIT_ASSERT(!redelitScalar->isLiteral());
}

void TypeCheckingTestCase::deliteralizeTuple() {
  //deliteralize(<Tuple <Any>...>) = <Tuple <Any>...>
  ProtoTuple* tup = new ProtoTuple();
  int size = tup->types.size();
  ProtoType* delit = Deliteralization::deliteralize(tup);
  CPPUNIT_ASSERT(delit->isA("ProtoTuple"));
  CPPUNIT_ASSERT(!delit->isLiteral());
  ProtoTuple* unlit = T_TYPE(delit);
  CPPUNIT_ASSERT(size == unlit->types.size() == 1);
  CPPUNIT_ASSERT(unlit->types[0]->isA("ProtoType"));
  CPPUNIT_ASSERT(!unlit->bounded);
  
  //deliteralize(<1-Tuple <Any>>) = <1-Tuple <Any>>
  tup = new ProtoTuple(true);
  tup->add(new ProtoType());
  size = tup->types.size();
  delit = Deliteralization::deliteralize(tup);
  CPPUNIT_ASSERT(delit->isA("ProtoTuple"));
  CPPUNIT_ASSERT(!delit->isLiteral());
  unlit = T_TYPE(delit);
  CPPUNIT_ASSERT(size == unlit->types.size() == 1);
  CPPUNIT_ASSERT(unlit->types[0]->isA("ProtoType"));
  CPPUNIT_ASSERT(unlit->bounded);
  
  //deliteralize(<1-Tuple <Scalar 2>>) = <1-Tuple <Scalar>>
  tup = new ProtoTuple(true);
  tup->add(new ProtoScalar(2));
  size = tup->types.size();
  delit = Deliteralization::deliteralize(tup);
  CPPUNIT_ASSERT(delit->isA("ProtoTuple"));
  CPPUNIT_ASSERT(!delit->isLiteral());
  unlit = T_TYPE(delit);
  CPPUNIT_ASSERT(size == unlit->types.size() == 1);
  CPPUNIT_ASSERT(unlit->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(!dynamic_cast<ProtoScalar*>(unlit->types[0])->isLiteral());
  CPPUNIT_ASSERT(unlit->bounded);
}

OperatorInstance* makeNewOI(vector<ProtoType*> req_input,
                            vector<ProtoType*> opt_input,
                            ProtoTuple* rest_input,
                            ProtoType* output) {
  Signature* sig = new Signature( new CE(),     //CE
                                  output ); //ProtoType
  for(int i=0; i<req_input.size(); i++) {
     sig->required_inputs.push_back(req_input[i]);
  }
  for(int i=0; i<opt_input.size(); i++) {
     sig->optional_inputs.push_back(opt_input[i]);
  }
  sig->rest_input = rest_input;
  Operator* op = new Operator( new CE(),  //CE
                               sig ); //Signature
  AmorphousMedium* space = new AmorphousMedium( new CE(),   //CE
                                                new DFG() ); //DFG
  OperatorInstance* oi = new OperatorInstance( new CE(),    //CE
                                               op,      //Operator
                                               space ); //AmorphousMedium
  for(int i=0; i<req_input.size(); i++) 
    oi->add_input( new Field(new CE(),space,req_input[i],oi) );
  for(int i=0; i<opt_input.size(); i++) 
    oi->add_input( new Field(new CE(),space,opt_input[i],oi) );
  if(rest_input) {
    for(int i=0; i<rest_input->types.size(); i++) {
      oi->add_input( new Field(new CE(),space,rest_input->types[i],oi) );
    }
  }
  return oi;
}

void TypeCheckingTestCase::getNthArg() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoTuple() );
  req_input.push_back( new ProtoScalar() );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  CPPUNIT_ASSERT(tca->get_nth_arg(oi, 0)->isA("ProtoTuple"));
  CPPUNIT_ASSERT(tca->get_nth_arg(oi, 1)->isA("ProtoScalar"));
  //CPPUNIT_ASSERT(tca->get_nth_arg(oi, 2) == NULL);
}

void TypeCheckingTestCase::getRefSymbol() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar() );
  req_input.push_back( new ProtoScalar(2) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  CPPUNIT_ASSERT(tca->get_ref(oi,new SE_Symbol("value"))->isA("ProtoTuple"));
  CPPUNIT_ASSERT(tca->get_ref(oi,new SE_Symbol("arg0"))->isA("ProtoScalar"));
  ProtoType* ref = tca->get_ref(oi,new SE_Symbol("arg1"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(ref->isLiteral());
  CPPUNIT_ASSERT(dynamic_cast<ProtoScalar*>(ref)->value == 2);
}

void TypeCheckingTestCase::getRefUnlit() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar() );
  req_input.push_back( new ProtoScalar(2) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("unlit"));
  sexpr->add(new SE_Symbol("arg1"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(!ref->isLiteral());
}

void TypeCheckingTestCase::getRefFieldof() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar() );
  req_input.push_back( new ProtoScalar(2) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("fieldof"));
  sexpr->add(new SE_Symbol("arg1"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoField"));
  CPPUNIT_ASSERT(F_TYPE(ref)->hoodtype->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(F_TYPE(ref)->hoodtype) == 2);
}

void TypeCheckingTestCase::getRefFt() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoField(new ProtoScalar(2)) );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("ft"));
  sexpr->add(new SE_Symbol("arg0"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 2);
}

void TypeCheckingTestCase::getRefInputs() {
  //First, make an oi as the op to a ProtoLambda
  ProtoLambda* lambda = NULL;
   {
      //required_inputs
      vector<ProtoType*> req_input = vector<ProtoType*>();
      req_input.push_back( new ProtoScalar() );
      req_input.push_back( new ProtoScalar(2) );
      //optional_inputs
      vector<ProtoType*> opt_input = vector<ProtoType*>();
      //rest
      ProtoTuple* rest = NULL;
      //output
      ProtoType* output = new ProtoTuple();
      OperatorInstance* oi = makeNewOI( req_input, 
                                        opt_input, 
                                        rest, 
                                        output );
      lambda = new ProtoLambda(oi->op);
   }
  //Now, add the ProtoLambda to a new OI
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( lambda );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("inputs"));
  sexpr->add(new SE_Symbol("arg0"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoTuple"));
  ProtoTuple* tup = T_TYPE(ref);
  CPPUNIT_ASSERT(tup->bounded);
  CPPUNIT_ASSERT(tup->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup->types[1]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(tup->types[1]) == 2);
}

void TypeCheckingTestCase::getRefOutput() {
  //First, make an oi as the op to a ProtoLambda
  ProtoLambda* lambda = NULL;
   {
      //required_inputs
      vector<ProtoType*> req_input = vector<ProtoType*>();
      req_input.push_back( new ProtoScalar() );
      req_input.push_back( new ProtoScalar(2) );
      //optional_inputs
      vector<ProtoType*> opt_input = vector<ProtoType*>();
      //rest
      ProtoTuple* rest = NULL;
      //output
      ProtoType* output = new ProtoScalar(4);
      OperatorInstance* oi = makeNewOI( req_input, 
                                        opt_input, 
                                        rest, 
                                        output );
      lambda = new ProtoLambda(oi->op);
   }
  //Now, add the ProtoLambda to a new OI
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( lambda );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = NULL;
  //output
  ProtoType* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("output"));
  sexpr->add(new SE_Symbol("arg0"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 4);
}

void TypeCheckingTestCase::getRefLast() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoField(new ProtoScalar(2)) );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar(7));
  rest->add(new ProtoScalar(8));
  //output
  ProtoTuple* output = new ProtoTuple(true);
  output->add( new ProtoScalar(4) );
  output->add( new ProtoScalar(5) );
  output->add( new ProtoScalar(6) );
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("last"));
  sexpr->add(new SE_Symbol("value"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 6);
  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("last"));
  sexpr->add(new SE_Symbol("arg2"));
  ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 8);
}

void TypeCheckingTestCase::getRefNth() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoField(new ProtoScalar(2)) );
  req_input.push_back( new ProtoScalar(1) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar(7));
  rest->add(new ProtoScalar(8));
  rest->add(new ProtoScalar(9));
  //output
  ProtoTuple* output = new ProtoTuple(true);
  output->add( new ProtoScalar(4) );
  output->add( new ProtoScalar(5) );
  output->add( new ProtoScalar(6) );
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("nth"));
  sexpr->add(new SE_Symbol("value"));
  sexpr->add(new SE_Symbol("arg1"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 5);
  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("nth"));
  sexpr->add(new SE_Symbol("arg2"));
  sexpr->add(new SE_Symbol("arg1"));
  ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 8);
}

void TypeCheckingTestCase::getRefLcs() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar(3) );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar(9));
  rest->add(new ProtoScalar(9));
  rest->add(new ProtoScalar(9));
  //output
  ProtoTuple* output = new ProtoTuple(true);
  output->add( new ProtoScalar(4) );
  output->add( new ProtoScalar(5) );
  output->add( new ProtoScalar(6) );
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("lcs"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg1"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(ref->isLiteral());
  CPPUNIT_ASSERT_EQUAL(3, (int)S_VAL(ref));
  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("lcs"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg1"));
  sexpr->add(new SE_Symbol("arg2"));
  ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(!ref->isLiteral());
  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("lcs"));
  sexpr->add(new SE_Symbol("arg2"));
  ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(ref->isLiteral());
  CPPUNIT_ASSERT_EQUAL(9, (int)S_VAL(ref));
}

void TypeCheckingTestCase::getRefTupof() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar(2) );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar(7));
  rest->add(new ProtoScalar(8));
  rest->add(new ProtoScalar(9));
  //output
  ProtoTuple* output = new ProtoTuple(true);
  output->add( new ProtoScalar(4) );
  output->add( new ProtoScalar(5) );
  output->add( new ProtoScalar(6) );
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  SE_List* sexpr = new SE_List();
  sexpr->add(new SE_Symbol("tupof"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg1"));
  ProtoType* ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoTuple"));
  ProtoTuple* tup = T_TYPE(ref);
  CPPUNIT_ASSERT(tup->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup->types[1]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(tup->types[0]) == 2);
  CPPUNIT_ASSERT(S_VAL(tup->types[1]) == 3);

  // rest alone
  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("tupof"));
  sexpr->add(new SE_Symbol("arg2"));
  ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoTuple"));
  tup = T_TYPE(ref);
  CPPUNIT_ASSERT(tup->types[0]->isA("ProtoTuple"));
  ProtoTuple* tup2 = T_TYPE(tup->types[0]);
  CPPUNIT_ASSERT(tup2->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup2->types[1]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup2->types[2]->isA("ProtoScalar"));
  CPPUNIT_ASSERT_EQUAL(7, (int)S_VAL(tup2->types[0]));
  CPPUNIT_ASSERT_EQUAL(8, (int)S_VAL(tup2->types[1]));
  CPPUNIT_ASSERT_EQUAL(9, (int)S_VAL(tup2->types[2]));
  
  // rest with others
  sexpr = new SE_List();
  sexpr->add(new SE_Symbol("tupof"));
  sexpr->add(new SE_Symbol("arg0"));
  sexpr->add(new SE_Symbol("arg1"));
  sexpr->add(new SE_Symbol("arg2"));
  ref = tca->get_ref(oi,sexpr);
  CPPUNIT_ASSERT(ref->isA("ProtoTuple"));
  tup = T_TYPE(ref);
  CPPUNIT_ASSERT(tup->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup->types[1]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(tup->types[0]) == 2);
  CPPUNIT_ASSERT(S_VAL(tup->types[1]) == 3);
  CPPUNIT_ASSERT(tup->types[2]->isA("ProtoTuple"));
  tup2 = T_TYPE(tup->types[2]);
  CPPUNIT_ASSERT(tup2->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup2->types[1]->isA("ProtoScalar"));
  CPPUNIT_ASSERT(tup2->types[2]->isA("ProtoScalar"));
  CPPUNIT_ASSERT_EQUAL(7, (int)S_VAL(tup2->types[0]));
  CPPUNIT_ASSERT_EQUAL(8, (int)S_VAL(tup2->types[1]));
  CPPUNIT_ASSERT_EQUAL(9, (int)S_VAL(tup2->types[2]));
}

void TypeCheckingTestCase::getRefOptionalInputs() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar(2) );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar(4) );
  opt_input.push_back( new ProtoScalar(5) );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar(7));
  rest->add(new ProtoScalar(8));
  rest->add(new ProtoScalar(9));
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  ProtoType* ref = tca->get_ref(oi,new SE_Symbol("arg0"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 2);
  ref = tca->get_ref(oi,new SE_Symbol("arg1"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 3);
  ref = tca->get_ref(oi,new SE_Symbol("arg2"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 4);
  ref = tca->get_ref(oi,new SE_Symbol("arg3"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 5);
  ref = tca->get_ref(oi,new SE_Symbol("arg4"));
  CPPUNIT_ASSERT(ref->isA("ProtoTuple"));
  CPPUNIT_ASSERT_EQUAL(7, (int)S_VAL(T_TYPE(ref)->types[0]));
  CPPUNIT_ASSERT_EQUAL(8, (int)S_VAL(T_TYPE(ref)->types[1]));
  CPPUNIT_ASSERT_EQUAL(9, (int)S_VAL(T_TYPE(ref)->types[2]));
}

void TypeCheckingTestCase::getRefRest() {
  //required_inputs
  vector<ProtoType*> req_input = vector<ProtoType*>();
  req_input.push_back( new ProtoScalar(2) );
  req_input.push_back( new ProtoScalar(3) );
  //optional_inputs
  vector<ProtoType*> opt_input = vector<ProtoType*>();
  opt_input.push_back( new ProtoScalar(4) );
  opt_input.push_back( new ProtoScalar(5) );
  //rest
  ProtoTuple* rest = new ProtoTuple(true);
  rest->add(new ProtoScalar(6));
  rest->add(new ProtoScalar(7));
  //output
  ProtoTuple* output = new ProtoTuple();
  OperatorInstance* oi = makeNewOI( req_input, 
                                    opt_input, 
                                    rest, 
                                    output );
  TypeConstraintApplicator* tca = new TypeConstraintApplicator(NULL);
  ProtoType* ref = tca->get_ref(oi,new SE_Symbol("arg0"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 2);
  ref = tca->get_ref(oi,new SE_Symbol("arg1"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 3);
  ref = tca->get_ref(oi,new SE_Symbol("arg2"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 4);
  ref = tca->get_ref(oi,new SE_Symbol("arg3"));
  CPPUNIT_ASSERT(ref->isA("ProtoScalar"));
  CPPUNIT_ASSERT(S_VAL(ref) == 5);
  ref = tca->get_ref(oi,new SE_Symbol("arg4"));
  CPPUNIT_ASSERT(ref->isA("ProtoTuple"));
  ProtoTuple* rest_elem = T_TYPE(ref);
  CPPUNIT_ASSERT(rest_elem->bounded);
  CPPUNIT_ASSERT_EQUAL(2, (int)rest_elem->types.size());
  CPPUNIT_ASSERT(rest_elem->types[0]->isA("ProtoScalar"));
  CPPUNIT_ASSERT_EQUAL(6, (int)S_VAL(rest_elem->types[0]));
  CPPUNIT_ASSERT_EQUAL(7, (int)S_VAL(rest_elem->types[1]));
}
