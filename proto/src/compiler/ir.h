/* Intermediate representation for compiler
Copyright (C) 2009, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// This is where the semantics of Proto are represented directly,
// and is the target for interpretation of textual Proto code.
// Type inference and most optimizations are performed on this
// representation.  After this stage, a program is transformed into
// first abstract bytecodes and then concrete bytecodes

#ifndef __IR__
#define __IR__

#include <set>
#include <vector>
#include <iostream>
#include "sexpr.h"

using namespace std;

// forward pointers:
struct DFG; struct Operator; struct OperatorInstance; struct Field;

/*****************************************************************************
 *  TYPES                                                                    *
 *****************************************************************************/
// Note: virtual inheritance requires dynamic_cast<type>(val) to downcast
struct ProtoType : public CompilationElement {
  virtual void print(ostream* out=0) { *out << "<Any>"; }
  virtual bool isA(string c){return c=="ProtoType"||CompilationElement::isA(c);}
  virtual string type_of() { return "ProtoType"; }
  // default means of testing for supertype-ness
  virtual bool supertype_of(ProtoType* sub){ return sub->isA(this->type_of()); }
  static bool equal(ProtoType* a, ProtoType* b)
  { return a->supertype_of(b) && b->supertype_of(a); }
  static ProtoType* lcs(ProtoType* a, ProtoType* b); // Least Common Supertype
  static ProtoType* gcs(ProtoType* a, ProtoType* b); // Greatest Common Subtype
  virtual ProtoType* lcs(ProtoType* t) { return new ProtoType(); }
  virtual ProtoType* gcs(ProtoType* t);
  virtual bool isLiteral() { return false; }
  virtual int pointwise() { return -1; } // uncertain whether it's pointwise
};

struct ProtoLocal : public ProtoType {
  ProtoLocal() {}
  virtual ~ProtoLocal() {}
  virtual void print(ostream* out=0) { *out << "<Local>"; }
  virtual bool isA(string c){return c=="ProtoLocal"||ProtoType::isA(c);}
  virtual string type_of() { return "ProtoLocal"; }
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
  virtual int pointwise() { return 1; } // definitely pointwise
};

struct ProtoTuple : virtual public ProtoLocal {
  vector<ProtoType*> types;
  bool bounded; // when false, last type is "rest" type
  ProtoTuple() { this->bounded=false; this->types.push_back(new ProtoType()); }
  ProtoTuple(bool bounded) { this->bounded=bounded; }
  ProtoTuple(ProtoTuple* src) { 
    bounded=src->bounded;
    for(int i=0;i<src->types.size();i++) add(src->types[i]); 
    inherit_attributes(src);
  }
  virtual ~ProtoTuple() {}
  void add(ProtoType* t) { types.push_back(t); }
  virtual void print(ostream* out=0);
  virtual bool isA(string c){return c=="ProtoTuple"||ProtoLocal::isA(c);}
  virtual string type_of() { return "ProtoTuple"; }
  virtual bool supertype_of(ProtoType* sub);
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
  virtual bool isLiteral() { 
    if(!bounded) return false;
    for(int i=0;i<types.size();i++) if(!types[i]->isLiteral()) return false;
    return true;
  }
};

struct ProtoSymbol : public ProtoLocal {
  bool constant; string value;
  ProtoSymbol() { constant=false; value=""; }
  ProtoSymbol(string v) { constant=true; value=v; }
  virtual void print(ostream* out=0) 
  { *out << "<Symbol"; if(constant) { *out << " " << value; } *out << ">"; }
  virtual bool isA(string c){return c=="ProtoSymbol"||ProtoLocal::isA(c);}
  virtual string type_of() { return "ProtoSymbol"; }
  virtual bool supertype_of(ProtoType* sub);
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
  virtual bool isLiteral() { return constant; }
};

struct ProtoNumber : virtual public ProtoLocal {
  virtual void print(ostream* out=0) { *out << "<Number>"; }
  virtual bool isA(string c){return c=="ProtoNumber"||ProtoLocal::isA(c);}
  virtual string type_of() { return "ProtoNumber"; }
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
};

struct ProtoScalar : public ProtoNumber {
  bool constant; float value;
  ProtoScalar() { constant=false; value=7734; }
  ProtoScalar(float v) { constant=true; value=v; }
  virtual ~ProtoScalar() {}
  virtual void print(ostream* out=0) 
  { *out << "<Scalar"; if(constant) { *out << " " << value; } *out << ">"; }
  virtual bool isA(string c){return c=="ProtoScalar"||ProtoNumber::isA(c);}
  virtual string type_of() { return "ProtoScalar"; }
  virtual bool supertype_of(ProtoType* sub);
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
  virtual bool isLiteral() { return constant; }
};

struct ProtoBoolean : public ProtoScalar {
  ProtoBoolean() { constant=false; value=7734; }
  ProtoBoolean(bool b) { constant=true; value=b; }
  virtual void print(ostream* out=0);
  virtual bool isA(string c){return c=="ProtoBoolean"||ProtoScalar::isA(c);}
  virtual string type_of() { return "ProtoBoolean"; }
  virtual ProtoType* lcs(ProtoType* t);
};

// Vectors are a Tuple of scalars
struct ProtoVector : public ProtoTuple, public ProtoNumber {
  ProtoVector() : ProtoTuple(false) {this->types.push_back(new ProtoScalar());}
  ProtoVector(bool bounded) : ProtoTuple(bounded) { }
  ProtoVector(ProtoVector* src) : ProtoTuple(src) { } 
  virtual ~ProtoVector() {}
  virtual void print(ostream* out=0);
  virtual bool isA(string c)
  {return c=="ProtoVector"||ProtoNumber::isA(c)||ProtoTuple::isA(c);}
  virtual string type_of() { return "ProtoVector"; }
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t) { return ProtoTuple::gcs(t); }
};

struct ProtoLambda: public ProtoLocal {
  Operator* op;
  ProtoLambda() { op=NULL; }
  ProtoLambda(Operator* op) { this->op = op; }
  virtual void print(ostream* out=0);
  virtual bool isA(string c){return c=="ProtoLambda"||ProtoLocal::isA(c);}
  virtual string type_of() { return "ProtoLambda"; }
  virtual bool supertype_of(ProtoType* sub);
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
  virtual bool isLiteral() { return op!=NULL; }
};

struct ProtoField : public ProtoType {
  ProtoType* hoodtype; // must be local, but derived types defer resolution
  ProtoField() { hoodtype=NULL; }
  ProtoField(ProtoType* hoodtype) { this->hoodtype = hoodtype; }
  virtual void print(ostream* out=0)
  { *out<<"<Field"; if(hoodtype){ *out<<" ";hoodtype->print(out);} *out<<">"; }
  virtual bool isA(string c){return c=="ProtoField"||ProtoType::isA(c);}
  virtual string type_of() { return "ProtoField"; }
  virtual bool supertype_of(ProtoType* sub);
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
  virtual bool isLiteral() { return hoodtype->isLiteral(); }
  virtual int pointwise() { return 0; } // definitely not pointwise
};

struct DerivedType : public ProtoType {
  SExpr* ref;
  DerivedType(SExpr* ref) { this->ref = ref; }
  static bool is_arg_ref(string s); // is this an arg ref?
  virtual void print(ostream* out=0)
  { *out<<"<D:"; ref->print(out); *out<<">"; }
  virtual bool isA(string c){return c=="DerivedType"||ProtoType::isA(c);}
  virtual string type_of() { return "DerivedType"; }
  // interpreter returns replacement, or NULL if still unresolvable
  ProtoType* resolve_type(OperatorInstance* oi) { return resolve_type(oi,ref); }
  virtual bool supertype_of(ProtoType* sub) { return false; }
  // LCS of derived-type punts right up to ProtoType
  virtual ProtoType* gcs(ProtoType* t);
 private:
  ProtoType* resolve_type(OperatorInstance* oi,SExpr* s);
};

/*****************************************************************************
 *  OPERATORS & MACROS                                                       *
 *****************************************************************************/
// SIGNATURES specify the I/O compatibility of an operator
struct Signature : public CompilationElement {
  ProtoType *output;
  vector<ProtoType*> required_inputs;
  vector<ProtoType*> optional_inputs;
  ProtoType* rest_input;
  Signature() { rest_input=NULL; }
  Signature(ProtoType* output) { rest_input=NULL; this->output=output; }
  bool legal_length(int n); // is n a permissible number of args?
  ProtoType* nth_type(int n);
  int n_fixed() { return required_inputs.size()+optional_inputs.size(); }
  virtual void print(ostream* out=0);
};

// OPERATORS are functions mapping from fields to fields
struct Operator : public CompilationElement {
  Signature *signature; 
  string name; // only set by some operators
  Operator() {}
  Operator(Signature *sig) { signature=sig; }
  string to_str() { ostringstream s; print(&s); return s.str(); }
  virtual void print(ostream* out=0) 
  { *out << "[Op"; signature->print(out); *out << "]"; }
  virtual string type_of() { return "Operator"; }
  virtual bool isA(string c)
  { return (c=="Operator")?true:CompilationElement::isA(c); }
};

struct Literal : public Operator {
  ProtoType *value;
  Literal(ProtoType *v) { 
    if(!v->isLiteral()) ierror("Tried to make Literal from "+v->to_str());
    signature = new Signature(v); value = v;
  }
  string to_str() { ostringstream s; print(&s); return s.str(); }
  virtual void print(ostream* out=0)
  {*out << "[Lit: "; value->print(out); *out << "]";}
  virtual bool isA(string c) { return (c=="Literal")?true:Operator::isA(c); }
};

struct Primitive : public Operator {
  Primitive() {} // used by FieldOp subclass, which initializes separately
  Primitive(string name, Signature* sig) { this->name=name; signature=sig; }
  virtual void print(ostream* out=0) { *out << "[" << name << "]"; }
  virtual bool isA(string c) { return (c=="Primitive")?true:Operator::isA(c); }
};

struct CompoundOp : public Operator {
  DFG *body;
  static int lambda_count; // uids for unnamed lambdas
  set<OperatorInstance*, CompilationElement_cmp> usages;
  // set body afterward, to allow recursiveness
  CompoundOp(string n);
  bool compute_side_effects(); // marks self & returns
  virtual void print(ostream* out=0)  {*out << "[Fun: " << name << "]";}
  virtual bool isA(string c){ return (c=="CompoundOp")?true:Operator::isA(c); }
};

struct Parameter : public Operator { // something input to a function
  ProtoType *defaultValue;
  CompoundOp* container;
  int index; // which signature variable is it?
  Parameter(CompoundOp* op, string name, int index, ProtoType* def=NULL) { 
    signature = new Signature(new ProtoType()); defaultValue = def; 
    this->name=name; this->index=index; container=op;
  }
  virtual void print(ostream* out=0);
  virtual bool isA(string c) { return (c=="Parameter")?true:Operator::isA(c); }
};

// A pointwise operator, "pushed down" to operator over field data types
struct FieldOp : public Primitive {
 private:
  static map<Operator*,FieldOp*,CompilationElement_cmp> fieldops;
  FieldOp(Operator* base);
 public:
  static FieldOp* get_field_op(OperatorInstance* oi); // null if can't convert
  
  Operator* base; // must be a pointwise primitive
  virtual bool isA(string c) { return (c=="FieldOp")?true:Primitive::isA(c); }
};

// MACROS are syntactic operations on S-Expressions
struct Macro : public CompilationElement {
  SExpr* pattern;
  string name;
  Macro(string name, SExpr* pattern) { this->name=name; this->pattern=pattern; }
  virtual void print(ostream* out=0)  {*out << "[Macro: " << name << "]";}
  virtual bool isA(string c){return c=="Macro"?true:CompilationElement::isA(c);}
};

struct MacroSymbol : public Macro {
 MacroSymbol(string name, SExpr* pattern) : Macro(name,pattern) {} 
  virtual string type_of() { return "MacroSymbol"; }
  virtual bool isA(string c) { return c=="MacroSymbol" ? true : Macro::isA(c); }
};

struct MacroOperator : public Macro {
  static int gensym_count;
  Signature *signature; // All ProtoSymbols w. constant value equal to var name
 MacroOperator(string name, Signature* sig, SExpr* pattern)
   : Macro(name,pattern) { signature=sig; }
  virtual string type_of() { return "MacroOperator"; }
  virtual bool isA(string c) { return c=="MacroOperator"?true:Macro::isA(c); }
};


/*****************************************************************************
 *  DATAFLOW GRAPHS                                                          *
 *****************************************************************************/

// AMORPHOUS MEDIUMS are the spaces where a program executes
struct AM : public CompilationElement {
  AM *parent; // nil if root
  Field *selector; // must be coercable to boolean
  // connections of this AM elsewhere
  set<Field*, CompilationElement_cmp> fields;
  set<AM*, CompilationElement_cmp> children;
  DFG* container; // the DFG this is in (set by constructor)
  AM(DFG* root) { parent=NULL; selector=NULL; container=root; } // root
  AM(AM* parent, Field* f);
  virtual void print(ostream* out=0);
  virtual bool child_of(AM* am); // is this a child of am?
};

// FIELDS are sets of data across space
struct Field : public CompilationElement {
  AM *domain;
  ProtoType *range;
  // connections of this field elsewhere
  OperatorInstance* producer;
  set<pair<OperatorInstance*,int>, CompilationElementIntPair_cmp > consumers;
  vector<AM*> selectors;
  DFG* container; // the DFG this is in (set by inherit_and_add)
  
  Field(AM *d,ProtoType *r,OperatorInstance *oi)
    { domain=d; range=r; producer=oi; domain->fields.insert(this); }
  virtual void print(ostream* out=0);
  virtual string type_of() { return "Field"; }
  virtual bool isA(string c){return c=="Field"?true:CompilationElement::isA(c);}
  // producer/consumer maintenance
  void use(OperatorInstance* oi,int i) { consumers.insert(make_pair(oi,i)); }
  void unuse(OperatorInstance* oi,int i) { consumers.erase(make_pair(oi,i)); }
};

// OPERATOR INSTANCES are computations performed on particular fields
struct OperatorInstance : public CompilationElement {
  Operator* op; // set by constructor
  vector<Field*> inputs;
  Field* output; // generated automatically
  DFG* container; // the DFG this is in (set by inherit_and_add)
  OperatorInstance(Operator *op, AM* space) {
    this->op=op; output = new Field(space,op->signature->output,this);
    if(op->isA("CompoundOp")) ((CompoundOp*)op)->usages.insert(this);
  }
  ProtoType* nth_input(int i); // get the range of the nth input (sets become vectors/tuples)
  ProtoType* output_type(); // get the output type
  Field* add_input(Field* f) // adds the field to the end
  { f->use(this,inputs.size()); inputs.push_back(f); return f; }
  Field* remove_input(int i) // disconnects the field
  { inputs[i]->unuse(this,i); return delete_at(&inputs,i); }
  virtual void print(ostream* out=0);
  
  // pointwise tests for operator space/time extent: 1=no, 0=yes, -1=unresolved
  int pointwise();
};

// A DATAFLOW GRAPH is a complete program
struct DFG : public CompilationElement {
  set<OperatorInstance*, CompilationElement_cmp> nodes;
  set<Field*, CompilationElement_cmp> edges;
  set<AM*, CompilationElement_cmp> spaces;
  map<Operator*,set<OperatorInstance*, CompilationElement_cmp>, 
    CompilationElement_cmp> funcalls;
  Field* output;
  CompoundOp* container; // set for CompoundOps
  
  DFG() { container = NULL; output = NULL; } // base state
  
  Field* inherit_and_add(CompilationElement* src, OperatorInstance* oi);
  void add_funcalls(CompoundOp* lambda, OperatorInstance* oi);
  string to_str() { ostringstream s; print(&s); return s.str(); }
  void print(ostream* out=0);
  void print_with_funcalls(ostream* out=0);
  
  DFG* instance(); // create an child copy of this DFG (for function manip)

  // DFG manipulation
  static void relocate_input(OperatorInstance* src, int src_loc, 
                             OperatorInstance* dst,int dst_loc);
  static void relocate_inputs(OperatorInstance* src, OperatorInstance* dst,
                              int insert);
  static void relocate_consumers(Field* src, Field* dst); // + selectors, output
  static void relocate_source(OperatorInstance* consumer,int in,Field* newsrc);
  void delete_node(OperatorInstance* oi);
  void delete_space(AM* am);
  bool make_op_inline(OperatorInstance* oi); // returns true if successful
};

/*****************************************************************************
 *  SPECIALIZED ERRORS                                                       *
 *****************************************************************************/
// These errors return a dummy expression, allowing the compiler to
// return and exit gracefully
CompilationElement* dummy(string type, CompilationElement* context);
Field* field_err(CompilationElement *where,string msg);
Operator* op_err(CompilationElement *where,string msg);
Macro* macro_err(CompilationElement *where,string msg);
ProtoType* type_err(CompilationElement *where,string msg);
Signature* sig_err(CompilationElement *where,string msg);
SExpr* sexp_err(CompilationElement *where,string msg);


#endif // __IR__
