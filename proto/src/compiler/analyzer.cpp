/* Proto optimizer
Copyright (C) 2009, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// The analyzer takes us from an initial interpretation to a concrete,
// optimized structure that's ready for compilation

#include "config.h"
#include "nicenames.h"
#include "compiler.h"

/*****************************************************************************
 *  PROPAGATOR                                                               *
 *****************************************************************************/

void queue_all_fields(DFG* g, Propagator* p) {
  p->worklist_f = g->edges;
  map<Operator*,set<OperatorInstance*> >::iterator i=g->funcalls.begin();
  for( ;i!=g->funcalls.end();i++) {
    CompoundOp* op = (CompoundOp*)((*i).first);
    p->worklist_f.insert(op->body->edges.begin(),op->body->edges.end());
  }
}

void queue_all_ops(DFG* g, Propagator* p) {
  p->worklist_o = g->nodes;
  map<Operator*,set<OperatorInstance*> >::iterator i=g->funcalls.begin();
  for( ;i!=g->funcalls.end();i++) {
    CompoundOp* op = (CompoundOp*)((*i).first);
    p->worklist_o.insert(op->body->nodes.begin(),op->body->nodes.end());
  }
}

void queue_all_ams(DFG* g, Propagator* p) {
  p->worklist_a = g->spaces;
  map<Operator*,set<OperatorInstance*> >::iterator i=g->funcalls.begin();
  for( ;i!=g->funcalls.end();i++) {
    CompoundOp* op = (CompoundOp*)((*i).first);
    p->worklist_a.insert(op->body->spaces.begin(),op->body->spaces.end());
  }
}

// neighbor marking:
enum { F_MARK=1, O_MARK=2, A_MARK=4 };
CompilationElement* src; set<CompilationElement*> queued;
void Propagator::queue_nbrs(Field* f, int marks) {
  if(marks&F_MARK || queued.count(f)) return;   queued.insert(f);
  if(f!=src) { if(act_fields) { worklist_f.insert(f); } marks |= F_MARK; }
  
  queue_nbrs(f->producer,marks); queue_nbrs(f->domain,marks);
  set<pair<OperatorInstance*,int> >::iterator i=f->consumers.begin();
  for( ;i!=f->consumers.end();i++) queue_nbrs((*i).first,marks);
  for(int i=0;i<f->selectors.size();i++) queue_nbrs(f->selectors[i],marks);
}
void Propagator::queue_nbrs(OperatorInstance* oi, int marks) {
  if(marks&O_MARK || queued.count(oi)) return;   queued.insert(oi);
  if(oi!=src) { if(act_ops) { worklist_o.insert(oi); } marks |= O_MARK; }

  queue_nbrs(oi->output,marks);
  for(int i=0;i<oi->inputs.size();i++) queue_nbrs(oi->inputs[i],marks);
  // if it's a compound op, queue the parameters & output
}
void Propagator::queue_nbrs(AM* am, int marks) {
  if(marks&A_MARK || queued.count(am)) return;   queued.insert(am);
  if(am!=src) { if(act_am) { worklist_a.insert(am); } marks |= A_MARK; }

  if(am==src) { // Fields & Ops don't affect one another through AM
    if(am->parent) queue_nbrs(am->parent,marks);
    if(am->selector) queue_nbrs(am->selector,marks);
    for(set<AM*>::iterator i=am->children.begin();i!=am->children.end();i++)
      queue_nbrs(*i,marks);
    for(set<Field*>::iterator i=am->fields.begin();i!=am->fields.end();i++)
      queue_nbrs(*i,marks);
  }
}

void Propagator::note_change(AM* am) 
{ queued.clear(); any_changes=true; src=am; queue_nbrs(am); }
void Propagator::note_change(Field* f) 
{ queued.clear(); any_changes=true; src=f; queue_nbrs(f); }
void Propagator::note_change(OperatorInstance* oi) 
{ queued.clear(); any_changes=true; src=oi; queue_nbrs(oi); }

void Propagator::maybe_set_range(Field* f,ProtoType* range) {
  if(!ProtoType::equal(f->range,range)) { 
    if(verbosity>=2) 
      *cpout<<"  Changing type of "<<f->to_str()<<" to "<<range->to_str()<<endl;
    f->range=range; note_change(f);
  }
}

bool Propagator::propagate(DFG* g) {
  if(verbosity>=1) *cpout << "Executing analyzer " << to_str() << endl;
  any_changes=false; root=g;
  // initialize worklists
  if(act_fields) queue_all_fields(g,this); else worklist_f.clear();
  if(act_ops) queue_all_ops(g,this); else worklist_o.clear();
  if(act_am) queue_all_ams(g,this); else worklist_a.clear();
  // walk through worklists until empty
  preprop();
  int steps_remaining = 
    loop_abort*(worklist_f.size()+worklist_o.size()+worklist_a.size());
  while(steps_remaining>0 && 
        (!worklist_f.empty() || !worklist_o.empty() || !worklist_a.empty())) {
    // each time through, try executing one from each worklist
    if(!worklist_f.empty()) {
      Field* f = *worklist_f.begin(); worklist_f.erase(f); 
      act(f); steps_remaining--;
    }
    if(!worklist_o.empty()) {
      OperatorInstance* oi = *worklist_o.begin(); worklist_o.erase(oi);
      act(oi); steps_remaining--;
    }
    if(!worklist_a.empty()) {
      AM* am = *worklist_a.begin(); worklist_a.erase(am); 
      act(am); steps_remaining--;
    }
  }
  if(steps_remaining<=0) ierror("Aborting due to apparent infinite loop.");
  postprop();
  return any_changes;
}

/*****************************************************************************
 *  TYPE CONCRETENESS                                                        *
 *****************************************************************************/

class Concreteness {
 public:
  // concreteness of fields comes from their types
  static bool acceptable(Field* f) { return acceptable(f->range); }
  // concreteness of types
  static bool acceptable(ProtoType* t) { 
    if(t->isA("ProtoScalar")) { return true;
    } else if(t->isA("ProtoSymbol")) { return true;
    } else if(t->isA("ProtoTuple")) { 
      ProtoTuple* tp = dynamic_cast<ProtoTuple*>(t);
      for(int i=0;i<tp->types.size();i++)
        if(!acceptable(tp->types[i])) return false;
      return true;
    } else if(t->isA("ProtoLambda")) { 
      ProtoLambda* tl = dynamic_cast<ProtoLambda*>(t); 
      return tl->op==NULL || acceptable(tl->op);
    } else if(t->isA("ProtoField")) {
      ProtoField* tf = dynamic_cast<ProtoField*>(t); 
      return tf->hoodtype==NULL || acceptable(tf->hoodtype);
    } else return false; // all others
  }
  // concreteness of operators, for ProtoLambda:
  static bool acceptable(Operator* op) { 
    if(op->isA("Literal") || op->isA("Parameter") || op->isA("Primitive")) {
      return true;
    } else if(op->isA("CompoundOp")) {
      CompoundOp* cop = dynamic_cast<CompoundOp*>(op); 
      set<Field*>::iterator fit;
      for(fit=cop->body->edges.begin(); fit!=cop->body->edges.end(); fit++)
        if(!acceptable(*fit)) return false;
      return true;
    } else return false; // generic operator
  }
};

void CheckTypeConcreteness::act(Field* f) {
  if(!Concreteness::acceptable(f->range))
    //ierror(f,"Type is ambiguous: "+f->to_str());
    compile_error(f,"Type is ambiguous: "+f->to_str());
}

/*****************************************************************************
 *  TYPE RESOLUTION                                                          *
 *****************************************************************************/

class TypeResolution {
 public:
  // resolution of fields comes from their types
  static bool acceptable(Field* f) { return acceptable(f->range); }
  // concreteness of types
  static bool acceptable(ProtoType* t) { 
    if(t->type_of()=="ProtoType") { return true;
    } else if(t->type_of()=="ProtoLocal") { return true;
    } else if(t->type_of()=="ProtoNumber") { return true;
    } else if(t->isA("ProtoScalar")) { return true;
    } else if(t->isA("ProtoSymbol")) { return true;
    } else if(t->isA("ProtoTuple")) { 
      ProtoTuple* tp = dynamic_cast<ProtoTuple*>(t);
      for(int i=0;i<tp->types.size();i++)
        if(!acceptable(tp->types[i])) return false;
      return true;
    } else if(t->isA("ProtoLambda")) { return true;
    } else if(t->isA("ProtoField")) {
      ProtoField* tf = dynamic_cast<ProtoField*>(t); 
      return tf->hoodtype==NULL || acceptable(tf->hoodtype);
    } else return false; // all others
  }
};



// used during type resolution
struct TypeVector : public ProtoTuple {
  //vector<ProtoType*> types;
  TypeVector() : ProtoTuple(true) {}
  virtual void print(ostream* out=0);
  virtual bool isA(string c){return c==type_of()||ProtoType::isA(c);}
  virtual string type_of() { return "TypeVector"; }
  // factory methods
  static TypeVector* op_types_from(OperatorInstance* oi, int start);
  static TypeVector* sig_type_range(Signature* s,int begin,int end);
  static TypeVector* input_types(Signature* sig);
  virtual bool supertype_of(ProtoType* sub) { 
    ierror("TypeVectors shouldn't be getting their types compared");
  }
};

TypeVector* TypeVector::op_types_from(OperatorInstance* oi, int start) {
  TypeVector *tv = new TypeVector();
  for(int i=start; i<oi->inputs.size(); i++)
    tv->types.push_back(oi->inputs[i]->range);
  return tv;
}

// returns types [begin,end-1]
TypeVector* TypeVector::sig_type_range(Signature* s,int begin,int end) {
  TypeVector *tv = new TypeVector();
  for(int i=begin; i<end; i++) tv->types.push_back(s->nth_type(i));
  return tv;
}

TypeVector* TypeVector::input_types(Signature* sig) {
  ierror("Haven't written signature typevec routing yet.");
}

void TypeVector::print(ostream* out) { 
  *out<<"<S:"; 
  for(int i=0;i<types.size();i++) { if(i) *out<<","; types[i]->print(out); }
  *out<<">";
}

/*
ProtoType* if_non_derived(TypeVector* tv) {
  for(int i=0;i<tv->types.size();i++)
    if(tv->types[i]->isA("DerivedType")) return NULL;
  return tv;
}
ProtoType* if_non_derived(ProtoType* t) { return t->isA("DerivedType")?NULL:t;}
*/

ProtoType* if_derivable(ProtoType* t, ProtoType* sigtype) {
  // don't derive from derived types
  if(t->isA("DerivedType")) return NULL;
  // don't derive from signature type mismatches
  if(!sigtype->isA("DerivedType") && !sigtype->supertype_of(t)) return NULL;
  return t;
}

ProtoType* if_derivable(TypeVector* tv, TypeVector* sigtypes) {
  for(int i=0;i<tv->types.size();i++)
    if(!if_derivable(tv->types[i],sigtypes->types[i])) return NULL;
  return tv;
}

// returns null to indicate a non-useful type
ProtoType* type_error(CompilationElement *where,string msg) {
  compile_error(where,"Type inference: "+msg); return NULL;
}

// return a (maybe copy) of the type 
ProtoType* deliteralize(ProtoType* base) {
  if(base->isA("ProtoVector")) {
    ProtoVector* t = dynamic_cast<ProtoVector*>(base);
    ProtoVector* newt = new ProtoVector(t->bounded);
    for(int i=0;i<t->types.size();i++)
      newt->types.push_back(deliteralize(t->types[i]));
    return newt;
  } else if(base->isA("ProtoTuple")) {
    ProtoTuple* t = dynamic_cast<ProtoTuple*>(base);
    ProtoTuple* newt = new ProtoTuple(t->bounded);
    for(int i=0;i<t->types.size();i++)
      newt->types.push_back(deliteralize(t->types[i]));
    return newt;
  } else if(base->isA("ProtoSymbol")) {
    ProtoSymbol* t = dynamic_cast<ProtoSymbol*>(base);
    return t->constant ? new ProtoSymbol() : t;
  } else if(base->isA("ProtoBoolean")) {
    ProtoBoolean* t = dynamic_cast<ProtoBoolean*>(base);
    return t->constant ? new ProtoBoolean() : t;
  } else if(base->isA("ProtoScalar")) {
    ProtoScalar* t = dynamic_cast<ProtoScalar*>(base);
    return t->constant ? new ProtoScalar() : t;
  } else if(base->isA("ProtoLambda")) {
    ierror("Deliteralization of ProtoLambdas is not yet implemented.");
  } else if(base->isA("ProtoField")) {
    ProtoField* t = dynamic_cast<ProtoField*>(base);
    return new ProtoField(deliteralize(t->hoodtype));
  } else {
    return base;
  }
}

// Type resolution needs to be applied to anything that isn't
// concrete...

ProtoType* Signature::nth_type(int n) {
  if(n < required_inputs.size()) return required_inputs[n];
  n-= required_inputs.size();
  if(n < optional_inputs.size()) return optional_inputs[n];
  n-= optional_inputs.size();
  if(rest_input) return rest_input;
  return type_error(this,"Can't find type"+i2s(n)+" in "+to_str());
}

ProtoType* OperatorInstance::nth_input(int n) {
  if(n < op->signature->n_fixed()) { // ordinary argument
    return if_derivable(inputs[n]->range,op->signature->nth_type(n));
  } else if(n==op->signature->n_fixed() && op->signature->rest_input) { // rest
    TypeVector *types = TypeVector::op_types_from(this,n),
      *sigt = TypeVector::sig_type_range(op->signature,n,inputs.size());
    return if_derivable(types,sigt);
  }
  return type_error(this,"Can't find input"+i2s(n)+" in "+to_str());
}

// assumes ref is already known to be an argref
// kth argument is actually combo of opinstance & signature nature.
// if signature includes a rest, then that arg returns a set
ProtoType* arg_ref(SE_Symbol* s, OperatorInstance* oi) {
  if(s->name=="args") {
    TypeVector* types = TypeVector::op_types_from(oi,0),
      *sigt = TypeVector::sig_type_range(oi->op->signature,0,oi->inputs.size());
    return if_derivable(types,sigt);
  }
  int n = atoi(s->name.substr(3,s->name.size()-3).c_str());
  return oi->nth_input(n);
}

// handles:
// produce singles: output, argK for non-rest
// produce sets: args, inputs, argK for a rest
// consume singles: ft, lcs
// consume sets: last, lcs, tupof
// special: nth: consumes (tuple or type-tuple) + scalar value

  /*
    Right now, I'm just going to track all of the reference types I use
    inputs, output: ops on a ProtoLambda
   */

ProtoType* DerivedType::resolve_type(OperatorInstance* oi, SExpr* ref) {
  if(ref->isSymbol()) {
    string var = ((SE_Symbol*)ref)->name;
    // ARGS: the set of argument types 
    // ARGK: the type of the kth argument (0-based)
    if(DerivedType::is_arg_ref(var)) { return arg_ref((SE_Symbol*)ref,oi);
    } else if(var=="return") {
      if(!oi->op->isA("CompoundOp")) 
        return type_error(ref,"'return' only valid for compound operators");
      ProtoType* rtype = ((CompoundOp*)oi->op)->body->output->range;
      return rtype->isA("DerivedType") ? NULL : rtype;
    } // else fall through to fail
  } else if(ref->isScalar()) { // expected to be only used inside nth
    return new ProtoScalar(((SE_Scalar*)ref)->value);
  } else if(ref->isList()) {
    SE_List* rl = (SE_List*)ref;
    string opname = ((SE_Symbol*)rl->op())->name;
    // first collect the types of the args, then process the op
    vector<ProtoType*> subs;
    for(int i=1;i<rl->len();i++) {
      ProtoType* sub = resolve_type(oi,(*rl)[i]);
      if(sub) subs.push_back(sub); else return NULL; // can't resolve yet
    }
    // NTH: finds the nth type in a tuple
    if(opname=="nth") {
      if(subs.size()!=2)
        return type_error(ref,"'nth' requires two arguments: "+ref->to_str());
      if(!subs[1]->isA("ProtoScalar"))
        return type_error(ref,"'nth' requires a scalar index: "+ref->to_str());
      ProtoScalar* n = dynamic_cast<ProtoScalar*>(subs[1]);
      vector<ProtoType*> *source;
      if(subs[0]->isA("TypeVector")) {
        source = &(dynamic_cast<TypeVector*>(subs[0]))->types;
      } else if(subs[0]->isA("ProtoTuple")) {
        source = &(dynamic_cast<ProtoTuple*>(subs[0]))->types;
      } else return type_error(oi,"'nth' needs tuple: "+subs[0]->to_str());
      if(source->size()==0) return type_error(oi,"'nth' source is empty");
      if(n->constant) { // if 2nd is a literal, access the argument
        if(n->value<0 || n->value>=source->size())
          return type_error(oi,"'nth' reference out of bounds: "+n->to_str());
        return (*source)[n->value];
      } else { // otherwise, take the LCS of the source
        ProtoType* compound = (*source)[0];
        for(int i=1;i<source->size();i++) 
          compound=ProtoType::lcs(compound,(*source)[i]);
        return compound;
      }
    // LCS: finds the least common superclass of set of types
    } else if(opname=="lcs") {
      vector<ProtoType*> tset; // collect everything into a set
      for(int i=0;i<subs.size();i++) {
        if(subs[i]->isA("TypeVector")) {
          TypeVector* tv = dynamic_cast<TypeVector*>(subs[i]);
          for(int i=0;i<tv->types.size();i++) { tset.push_back(tv->types[i]); }
        } else tset.push_back(subs[i]);
      }
      if(tset.size()==0) return type_error(oi,"'lcs' needs at least one type");
      ProtoType* compound = tset[0];
      for(int i=1;i<tset.size();i++) compound=ProtoType::lcs(compound,tset[i]);
      return compound;
    // LAST: find the last type in a set
    } else if(opname=="last") {
      if(subs.size()!=1 || !subs[0]->isA("TypeVector"))
        return type_error(ref,"'last' takes a single set: "+ref->to_str());
      TypeVector* tv = dynamic_cast<TypeVector*>(subs[0]);
      if(tv->types.size()==0) 
        return type_error(oi,"'last' takes a set with at least one type");
      return tv->types[tv->types.size()-1];
    // UNLIT: same type, but any literal value is discarded
    } else if(opname=="unlit") { // remove literalness from a type
      if(subs.size()!=1 || subs[0]->isA("TypeVector"))
        return type_error(ref,"'unlit' takes a single type: "+ref->to_str());
      return deliteralize(subs[0]);
    // TUPOF: tuple w. a set of types as arguments
    } else if(opname=="tupof") {
      if(subs.size()!=1 || !subs[0]->isA("TypeVector"))
        return type_error(ref,"'tupof' takes a single set: "+ref->to_str());
      TypeVector* tv = dynamic_cast<TypeVector*>(subs[0]);
      ProtoTuple* t = new ProtoTuple(true);
      for(int i=0;i<tv->types.size();i++) t->types.push_back(tv->types[i]);
      return t;
    // FT: type of local contained by a field
    } else if(opname=="ft") {
      if(subs.size()!=1 || !subs[0]->isA("ProtoField"))
        return type_error(oi,"'ft' takes a single field: "+v2s(&subs));
      ProtoField* t = dynamic_cast<ProtoField*>(subs[0]);
      return t->hoodtype;
    // FIELDOF: field containing a local
    } else if(opname=="fieldof") {
      if(subs.size()!=1 || !subs[0]->isA("ProtoLocal"))
        return type_error(oi,"'fieldof' takes a single local: "+ref->to_str());
      return new ProtoField(subs[0]);
    } else if(opname=="output") {
      if(subs.size()!=1 || !subs[0]->isA("ProtoLambda"))
        return type_error(oi,"'output' takes a single lambda: "+ref->to_str());
      ProtoLambda* t = dynamic_cast<ProtoLambda*>(subs[0]);
      ProtoType* rtype = NULL;
      if(t->op->isA("Primitive")) { rtype = t->op->signature->output;
      } else if(t->op->isA("CompoundOp")) { 
        rtype = ((CompoundOp*)oi->op)->body->output->range;
      } else {
        ierror("'output' derivation can't resolve for op: "+t->op->to_str());
      }
      return (TypeResolution::acceptable(rtype) ? rtype : NULL);
    } else if(opname=="inputs") {
      if(subs.size()!=1 || !subs[0]->isA("ProtoLambda"))
        return type_error(oi,"'inputs' takes a single lambda: "+ref->to_str());
      ProtoLambda* t = dynamic_cast<ProtoLambda*>(subs[0]);
      ProtoType* rtype = NULL;
      if(t->op->isA("Primitive")) { 
        rtype = TypeVector::input_types(t->op->signature);
      } else if(t->op->isA("CompoundOp")) {
        ierror("input derivation not yet done for compoundops");
      } else {
        ierror("'inputs' derivation can't resolve for op: "+t->op->to_str());
      }
      return (TypeResolution::acceptable(rtype) ? rtype : NULL);
    } // else fall through to fail
  } // fall throughs fail:
  ierror(ref,"Can't infer type from: "+ref->to_str()); // temporary, to help test suite
  return type_error(ref,"Can't infer type from: "+ref->to_str());
}

// returns type only if all can resolve to concrete; guaranteed to not
// corrupt the input type
ProtoType* resolve_type(OperatorInstance* context, ProtoType* type) {
  if(TypeResolution::acceptable(type)) return type; // already resolved
  // either is a derived type or contains a derived type:
  if(type->isA("DerivedType")) {
    DerivedType* t = dynamic_cast<DerivedType*>(type);
    return t->resolve_type(context);
  } else if(type->isA("ProtoTuple")) {
    ProtoTuple* t = dynamic_cast<ProtoTuple*>(type), *newt;
    if(type->isA("ProtoVector")) {
      newt = new ProtoVector(dynamic_cast<ProtoTuple*>(t));
    } else { newt = new ProtoTuple(t); }
    for(int i=0;i<newt->types.size();i++) {
      if(TypeResolution::acceptable(newt->types[i])) continue;
      ProtoType* newsub = resolve_type(context,newt->types[i]);
      if(!newsub) return NULL; else newt->types[i] = newsub;
    }
    return newt;
  } else if(type->isA("ProtoField")) {
    ProtoField* t = dynamic_cast<ProtoField*>(type);
    ProtoType* newsub = resolve_type(context,t->hoodtype);
    if(!newsub) return NULL;
    ProtoField* newf = new ProtoField(newsub); newf->inherit_attributes(t);
    return newf;
  }
  return NULL; // unresolvable by this means
}

class TypePropagator : public Propagator {
 public:
  TypePropagator(ProtoAnalyzer* parent, Args* args) : Propagator(true,true) {
    verbosity = args->extract_switch("--type-propagator-verbosity") ? 
      args->pop_number() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "TypePropagator"; }
  
  // apply a consumer constraint, managing implicit type conversions 
  bool back_constraint(ProtoType** tmp,Field* f,pair<OperatorInstance*,int> c) {
    ProtoType* rawc = c.first->op->signature->nth_type(c.second);
    ProtoType* ct = resolve_type(c.first,rawc);
    if(!ct) return true;// ignore unresolved
    ProtoType* newtype = ProtoType::gcs(*tmp,ct);
    if(verbosity>=3) *cpout<<"   Back constraint on: "<<c.first->to_str()<<
                       "\n    "<<(*tmp)->to_str()<<" vs. "<<ct->to_str()<<"...";
    if(newtype) {
      if(verbosity>=3) *cpout << " ok" << endl; 
      *tmp = newtype; return true; 
    }
    if(verbosity>=3) *cpout<<" FAIL"<<endl;

    // On merge failure, either add an implicit type conversion or error
    // if vector is needed and scalar is provided, convert to a size-1 vector
    if((*tmp)->isA("ProtoScalar") && ct->isA("ProtoVector")) {
      if(verbosity>=2) *cpout<<"Converting Scalar to 1-Vector\n";
      OperatorInstance* tup = new OperatorInstance(Env::TUP,f->domain);
      f->container->inherit_and_add(f->producer,tup);
      tup->add_input(f);
      f->container->relocate_source(c.first,c.second,tup->output);
      note_change(tup); return false; // let constraint retry later...
    }

    // if source is local and user wants a field, add a "local" op
    // or replace no-argument source with a field op
    if((*tmp)->isA("ProtoLocal") && ct->isA("ProtoField")) {
      ProtoField* ft = dynamic_cast<ProtoField*>(ct);

      if(!ft->hoodtype || ProtoType::gcs(*tmp,ft->hoodtype)) {
        FieldOp* fo = FieldOp::get_field_op(f->producer);
        if(f->producer->inputs.size()==0 && f->producer->pointwise()!=0 && fo) {
          if(verbosity>=2) *cpout<<"    Converting pointwise to field op\n";
          OperatorInstance* foi = new OperatorInstance(fo,f->domain);
          f->container->inherit_and_add(f->producer,foi);
          f->container->relocate_source(c.first,c.second,foi->output);
          note_change(foi); return false; // let constraint retry later...
        } else {
          if(verbosity>=2)*cpout<<"   Inserting 'local' at "<<f->to_str()<<endl;
          OperatorInstance* local = new OperatorInstance(Env::LOCAL,f->domain);
          f->container->inherit_and_add(f->producer,local);
          local->add_input(f);
          f->container->relocate_source(c.first,c.second,local->output);
          note_change(local); return false; // let constraint retry later...
        }
      }
    }
    
    // if source is field and user is "local", send to the local's consumers
    if((*tmp)->isA("ProtoField") && c.first->op==Env::LOCAL) {
      if(verbosity>=2) *cpout<<"   Deleting 'local' at "<<f->to_str()<<endl;
      f->container->relocate_consumers(c.first->output,f);
      return false;
    }
    
    // if source is field and user is pointwise, upgrade to field op
    if((*tmp)->isA("ProtoField") && ct->isA("ProtoLocal")) {
      ProtoField* ft = dynamic_cast<ProtoField*>(*tmp);
      int pw = c.first->pointwise();
      if(pw!=0 && (!ft->hoodtype || ProtoType::gcs(ct,ft->hoodtype))) {
        FieldOp* fo = FieldOp::get_field_op(c.first); // might be upgradable
        if(fo) {
          if(verbosity>=2) *cpout<<"    Converting pointwise to field op\n";
          c.first->op = fo; note_change(c.first);
        }
        return false; // in any case, don't fail out now
      }
    }
  
    // having fallen through all cases, throw type error
    type_error(f,"conflict: "+f->to_str()+" vs. "+c.first->to_str());
    return false;
  }

  void act(Field* f) {
    if(verbosity>=3) *cpout << " Considering field "<<f->to_str()<<endl;
    // Ignore old type (it may change) [except Parameter]; use producer type
    ProtoType* tmp=resolve_type(f->producer,f->producer->op->signature->output);
    if(f->producer->op->isA("Parameter")) tmp=f->range;
    if(!tmp) return; // if producer unresolvable, give up
    // GCS against consumers
    set<pair<OperatorInstance*,int> >::iterator i=f->consumers.begin();
    for( ;i!=f->consumers.end();i++)
      if(!back_constraint(&tmp,f,*i)) return; // type problem handled within
    // GCS against selectors
    if(f->selectors.size()) {
      tmp = ProtoType::gcs(tmp,new ProtoScalar());
      if(!tmp) {type_error(f,"non-scalar selector "+f->to_str()); return;}
    }
    maybe_set_range(f,tmp);
    
    /*
    if(f->range->type_of()=="ProtoTuple") { // look for tuple->vector upgrades
      ProtoTuple* tt = dynamic_cast<ProtoTuple*>(f->range);
      for(int i=0;i<tt->types.size();i++) 
        if(!tt->types[i]->isA("ProtoScalar")) return; // all scalars?
      ProtoVector* v = new ProtoVector(tt->bounded); v->types = tt->types;
      maybe_set_range(f,v);
    }
    */
  }

  // At each round, consider types of all neighbors... 
  // GCS against each that resolves
  // Resolve shouldn't care about the current type!

  void act(OperatorInstance* oi) {
    if(verbosity>=3) *cpout << " Considering op instance "<<oi->to_str()<<endl;
     //check if number of inputs is legal
    if(!oi->op->signature->legal_length(oi->inputs.size())) {
      compile_error(oi,oi->to_str()+" has "+i2s(oi->inputs.size())+" argumen"
                    "ts, but signature is "+oi->op->signature->to_str());
      return;
    }
    
    if(oi->op->isA("Primitive")) { // constrain against signature
      ProtoType* newtype = resolve_type(oi,oi->output->range);
      if(newtype) { maybe_set_range(oi->output,newtype);
      } else if(oi->attributes.count("LETFED-MUX") &&
                oi->output->range->isA("DerivedType")) {
        // letfed mux resolves from init
        if(oi->inputs.size()<2)
          type_error(oi,"Can't resolve letfed type: not enough mux arguments");
        Field* init = oi->inputs[1]; // true input = init
        if(!init->range->isA("DerivedType"))
          maybe_set_range(oi->output,deliteralize(init->range));
      }

      // ALSO: find GCS of producer, consumers, & field values
    } else if(oi->op->isA("Parameter")) { // constrain against all calls
      // find LCS of input types
      ProtoType* inputs = NULL;
      set<OperatorInstance*> *srcs = 
        &root->funcalls[((Parameter*)oi->op)->container];
      for(set<OperatorInstance*>::iterator i=srcs->begin();i!=srcs->end();i++) {
        ProtoType* ti = (*i)->nth_input(((Parameter*)oi->op)->index);
        if(!ti) return; // can't work with derived types
        inputs = inputs? ProtoType::lcs(inputs,ti) : ti;
      }
      if(!inputs) return;
      // then take GCS of that against current field value
      ProtoType* newtype = ProtoType::gcs(oi->output->range,inputs);
      if(!newtype) {compile_error(oi,"type conflict for "+oi->to_str());return;}
      maybe_set_range(oi->output,newtype);
    } else if(oi->op->isA("CompoundOp")) { // constrain against params & output
      ProtoType* rtype = ((CompoundOp*)oi->op)->body->output->range;
      if(!rtype->isA("DerivedType")) maybe_set_range(oi->output,rtype);
    } else if(oi->op->isA("Literal")) { // ignore: already be fully resolved
      // ignored
    } else {
      ierror("Don't know how to do type inference on undefined operators");
    }
  }
};

/*****************************************************************************
 *  CONSTANT FOLDING                                                         *
 *****************************************************************************/

// NOTE: constantfolder is *not* a type checker!

// scalars & shorter vectors are treated as having 0s all the way out
int compare_numbers(ProtoType* a, ProtoType* b) { // 1-> a>b; -1-> a<b; 0-> a=b
  if(a->isA("ProtoScalar") && b->isA("ProtoScalar")) {
    double va = (dynamic_cast<ProtoScalar*>(a))->value;
    double vb = (dynamic_cast<ProtoScalar*>(b))->value;
    return (va==vb) ? 0 : ((va>vb) ? 1 : -1);
  } else if(a->isA("ProtoScalar") || b->isA("ProtoScalar")) {
    int sx = a->isA("ProtoScalar") ? -1 : 1;
    double s = (dynamic_cast<ProtoScalar*>(a->isA("ProtoScalar")?a:b))->value;
    ProtoTuple* v = dynamic_cast<ProtoTuple*>(a->isA("ProtoScalar")?b:a);
    ProtoScalar* s0 = dynamic_cast<ProtoScalar*>(v->types[0]);
    if(s==s0->value) {
      if(v->types.size()>=2) {
        ProtoScalar* s1 = dynamic_cast<ProtoScalar*>(v->types[0]);
        return sx * ((s1->value == 0) ? 0 : ((s1->value > 0) ? 1 : -1));
      } else return 0;
    } else return (sx * ((s0->value > s) ? 1 : -1));
  } else { // 2 vectors
    ProtoTuple* va = dynamic_cast<ProtoTuple*>(a);
    ProtoTuple* vb = dynamic_cast<ProtoTuple*>(b);
    int la = va->types.size(), lb = vb->types.size(), len = MAX(la,lb);
    for(int i=0;i<len;i++) {
      double sa = (i<la)?(dynamic_cast<ProtoScalar*>(va->types[i]))->value:0;
      double sb = (i<lb)?(dynamic_cast<ProtoScalar*>(vb->types[i]))->value:0;
      if(sa!=sb) { return (sa>sb) ?  1 : -1; }
    }
    return 0;
  }
}

ProtoNumber* add_consts(ProtoType* a, ProtoType* b) {
  if(a->isA("ProtoScalar") && b->isA("ProtoScalar")) {
    double va = (dynamic_cast<ProtoScalar*>(a))->value;
    double vb = (dynamic_cast<ProtoScalar*>(b))->value;
    return new ProtoScalar(va+vb);
  } else if(a->isA("ProtoScalar") || b->isA("ProtoScalar")) {
    double s = (dynamic_cast<ProtoScalar*>(a->isA("ProtoScalar")?a:b))->value;
    ProtoTuple* v = dynamic_cast<ProtoTuple*>(a->isA("ProtoScalar")?b:a);
    ProtoVector* out = new ProtoVector(v->bounded);
    for(int i=0;i<v->types.size();i++) out->add(v->types[i]);
    (dynamic_cast<ProtoScalar*>(v->types[0]))->value += s;
    return out;
  } else { // 2 vectors
    ProtoTuple* va = dynamic_cast<ProtoTuple*>(a);
    ProtoTuple* vb = dynamic_cast<ProtoTuple*>(b);
    ProtoVector* out = new ProtoVector(va->bounded && vb->bounded);
    int la = va->types.size(), lb = vb->types.size(), len = MAX(la,lb);
    for(int i=0;i<len;i++) {
      double sa = (i<la)?(dynamic_cast<ProtoScalar*>(va->types[i]))->value:0;
      double sb = (i<lb)?(dynamic_cast<ProtoScalar*>(vb->types[i]))->value:0;
      out->add(new ProtoScalar(sa+sb));
    }
    return out;
  }
}

class ConstantFolder : public Propagator {
 public:
  ConstantFolder(ProtoAnalyzer* parent, Args* args) : Propagator(false,true) {
    verbosity = args->extract_switch("--constant-folder-verbosity") ? 
      args->pop_number() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "ConstantFolder"; }

  // FieldOp compatible accessors
  ProtoType* nth_type(OperatorInstance* oi, int i) {
    ProtoType* src = oi->inputs[i]->range;
    if(oi->op->isA("FieldOp")) src = dynamic_cast<ProtoField*>(src)->hoodtype;
    return src;
  }
  double nth_scalar(OperatorInstance* oi, int i) 
  { return dynamic_cast<ProtoScalar*>(nth_type(oi,i))->value; }
  ProtoTuple* nth_tuple(OperatorInstance* oi, int i) 
  { return dynamic_cast<ProtoTuple*>(nth_type(oi,i)); }
  void maybe_set_output(OperatorInstance* oi,ProtoType* content) {
    if(oi->op->isA("FieldOp")) content = new ProtoField(content);
    maybe_set_range(oi->output,content);
  }

  void act(OperatorInstance* oi) {
    if(!oi->op->isA("Primitive")) return; // only operates on primitives
    //if(oi->output->range->isLiteral()) return; // might change...
    string name = ((Primitive*)oi->op)->name;
    if(oi->op->isA("FieldOp")) name = ((FieldOp*)oi->op)->base->name;
    // handled by type inference: elt, min, max, tup, all argk pass-throughs
    if(name=="mux") { 
      if(oi->inputs[0]->range->isLiteral()) { // case 1: arg0 is literal
        maybe_set_output(oi,oi->inputs[nth_scalar(oi,0) ? 1 : 2]->range);
      } else if(oi->inputs[1]->range->isLiteral() && // case 2: branches equal
                ProtoType::equal(oi->inputs[1]->range,oi->inputs[2]->range)) {
        maybe_set_output(oi,oi->inputs[1]->range);
      }
      return;
    } else if(name=="len") { // len needs only tuple to be bounded
      ProtoTuple* tt = nth_tuple(oi,0);
      if(tt->bounded) 
        maybe_set_output(oi,new ProtoScalar(tt->types.size()));
      return;
    }

    // rest only apply if all inputs are literals
    for(int i=0;i<oi->inputs.size();i++) {
      if(!oi->inputs[i]->range->isLiteral()) return;
    }
    if(name=="not") {
      maybe_set_output(oi,new ProtoBoolean(!nth_scalar(oi,0)));
    } else if(name=="+") {
      ProtoNumber* sum = new ProtoScalar(0);
      for(int i=0;i<oi->inputs.size();i++) sum=add_consts(sum,nth_type(oi,i));
      maybe_set_output(oi,sum);
    } else if(name=="-") {
      ProtoNumber* sum = new ProtoScalar(0); // sum all but first
      for(int i=1;i<oi->inputs.size();i++) 
        sum=add_consts(sum,nth_type(oi,i));
      // multiply by negative 1
      if(sum->isA("ProtoScalar")) {
        ProtoScalar* s = dynamic_cast<ProtoScalar*>(sum); s->value *= -1;
      } else { // vector
        ProtoVector* s = dynamic_cast<ProtoVector*>(sum); 
        for(int i=0;i<s->types.size();i++)
          (dynamic_cast<ProtoScalar*>(s->types[i]))->value *= -1;
      }
      // add in first
      sum = add_consts(sum,nth_type(oi,0));
      maybe_set_output(oi,sum);
    } else if(name=="*") {
      double mults = 1;
      ProtoTuple* vnum = NULL;
      for(int i=1;i<oi->inputs.size();i++) {
        if(nth_type(oi,i)->isA("ProtoScalar")) { mults *= nth_scalar(oi,i);
        } else if(vnum) { compile_error(oi,">1 vector in multiplication");
        } else { vnum = nth_tuple(oi,i); 
        }
      }
      // final optional stage of vector multiplication
      if(vnum) {
        ProtoVector *pv = new ProtoVector(vnum->bounded);
        for(int i=0;i<vnum->types.size();i++) {
          double numi = (dynamic_cast<ProtoScalar*>(vnum->types[i]))->value;
          pv->add(new ProtoScalar(numi*mults));
        }
        maybe_set_output(oi,pv);
      } else {
        maybe_set_output(oi,new ProtoScalar(mults));
      }
    } else if(name=="/") {
      double denom = 1;
      for(int i=1;i<oi->inputs.size();i++) denom *= nth_scalar(oi,i);
      if(oi->inputs[0]->range->isA("ProtoScalar")) {
        double num = nth_scalar(oi,0);
        maybe_set_output(oi,new ProtoScalar(num/denom));
      } else if(oi->inputs[0]->range->isA("ProtoVector")) { // vector
        ProtoTuple *num = nth_tuple(oi,0);
        ProtoVector *pv = new ProtoVector(num->bounded);
        for(int i=0;i<num->types.size();i++) {
          double numi = (dynamic_cast<ProtoScalar*>(num->types[i]))->value;
          pv->add(new ProtoScalar(numi/denom));
        }
        maybe_set_output(oi,pv);
      }
    } else if(name==">") {
      int cp = compare_numbers(nth_type(oi,0),nth_type(oi,1));
      maybe_set_output(oi,new ProtoBoolean(cp==1));
    } else if(name=="<") {
      int cp = compare_numbers(nth_type(oi,0),nth_type(oi,1));
      maybe_set_output(oi,new ProtoBoolean(cp==-1));
    } else if(name=="=") {
      int cp = compare_numbers(nth_type(oi,0),nth_type(oi,1));
      maybe_set_output(oi,new ProtoBoolean(cp==0));
    } else if(name=="<=") {
      int cp = compare_numbers(nth_type(oi,0),nth_type(oi,1));
      maybe_set_output(oi,new ProtoBoolean(cp!=1));
    } else if(name==">=") {
      int cp = compare_numbers(nth_type(oi,0),nth_type(oi,1));
      maybe_set_output(oi,new ProtoBoolean(cp!=-1));
    } else if(name=="abs") {
      maybe_set_output(oi,new ProtoScalar(fabs(nth_scalar(oi,0))));
    } else if(name=="floor") {
      maybe_set_output(oi,new ProtoScalar(floor(nth_scalar(oi,0))));
    } else if(name=="ceil") {
      maybe_set_output(oi,new ProtoScalar(ceil(nth_scalar(oi,0))));
    } else if(name=="round") {
      // (primitive round (scalar) scalar)
      compile_warn(oi,"ConstantFolder incomplete for "+oi->op->to_str());
    } else if(name=="mod") {
      double a = nth_scalar(oi,0), b = nth_scalar(oi,1);
      maybe_set_output(oi,new ProtoScalar(fmod(a,b)));
    } else if(name=="pow") {
      double a = nth_scalar(oi,0), b = nth_scalar(oi,1);
      maybe_set_output(oi,new ProtoScalar(pow(a,b)));
    } else if(name=="sqrt") { 
      maybe_set_output(oi,new ProtoScalar(sqrt(nth_scalar(oi,0))));
    } else if(name=="log") {
      maybe_set_output(oi,new ProtoScalar(log(nth_scalar(oi,0))));
    } else if(name=="sin") {
      maybe_set_output(oi,new ProtoScalar(sin(nth_scalar(oi,0))));
    } else if(name=="cos") {
      maybe_set_output(oi,new ProtoScalar(cos(nth_scalar(oi,0))));
    } else if(name=="tan") {
      maybe_set_output(oi,new ProtoScalar(tan(nth_scalar(oi,0))));
    } else if(name=="asin") {
      maybe_set_output(oi,new ProtoScalar(asin(nth_scalar(oi,0))));
    } else if(name=="acos") {
      maybe_set_output(oi,new ProtoScalar(acos(nth_scalar(oi,0))));
    } else if(name=="atan2") {
      double a = nth_scalar(oi,0), b = nth_scalar(oi,1);
      maybe_set_output(oi,new ProtoScalar(atan2(a,b)));
    } else if(name=="sinh") {
      maybe_set_output(oi,new ProtoScalar(sinh(nth_scalar(oi,0))));
    } else if(name=="cosh") {
      maybe_set_output(oi,new ProtoScalar(cosh(nth_scalar(oi,0))));
    } else if(name=="tanh") {
      maybe_set_output(oi,new ProtoScalar(tanh(nth_scalar(oi,0))));
    } else if(name=="vdot") {
      // (primitive vdot (vector vector) scalar)
      compile_warn(oi,"ConstantFolder incomplete for "+oi->op->to_str());
    } else if(name=="min-hood" || name=="max-hood" || name=="any-hood"
              || name=="all-hood") {
      ProtoType* src = oi->inputs[0]->range;
      maybe_set_output(oi,dynamic_cast<ProtoField*>(src)->hoodtype);
    }
  }
  
};

/*****************************************************************************
 *  LITERALIZER                                                              *
 *****************************************************************************/

class Literalizer : public Propagator {
 public:
  Literalizer(ProtoAnalyzer* parent, Args* args) : Propagator(true,true) {
    verbosity = args->extract_switch("--literalizer-verbosity") ? 
      args->pop_number() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "Literalizer"; }
  void act(Field* f) {
    if(f->producer->op->isA("Literal")) return; // literals are already set
    if(f->producer->op->attributes.count(":side-effect")) return; // keep sides
    if(f->range->isLiteral()) {
      Operator* lit = new Literal(f->range);
      OperatorInstance* oi = new OperatorInstance(lit,f->domain);
      f->container->inherit_and_add(f->producer,oi);
      if(verbosity>=2)
        *cpout<<" Literalizing:\n  "<<f->producer->to_str()<<" \n  into "
              <<oi->to_str()<<endl;
      DFG::relocate_consumers(f,oi->output); note_change(f);
      f->container->delete_node(f->producer); // kill old op
    }
  }

  void act(OperatorInstance* oi) {
    if(!oi->op->isA("Primitive")) return; // only operates on primitives
    string name = ((Primitive*)oi->op)->name;
    // change "apply" of literal lambda into just an OI of that operator
    if(name=="apply") {
      if(oi->inputs[0]->range->isA("ProtoLambda") && 
         oi->inputs[0]->range->isLiteral()) {
        ProtoLambda* lf = dynamic_cast<ProtoLambda*>(oi->inputs[0]->range);
        OperatorInstance* newoi=new OperatorInstance(lf->op,oi->output->domain);
        for(int i=1;i<oi->inputs.size();i++) newoi->add_input(oi->inputs[i]);
        oi->container->inherit_and_add(oi,newoi);
        if(verbosity>=2)
          *cpout<<" Literalizing:\n  "<<oi->to_str()<<" \n  into "
                <<newoi->to_str()<<endl;
        DFG::relocate_consumers(oi->output,newoi->output); note_change(oi);
        oi->container->delete_node(oi); // kill old op
      }
    }
  }
};

/*****************************************************************************
 *  DEAD CODE ELIMINATOR                                                     *
 *****************************************************************************/

//  - mark the output and each Operator w. a side-effect attribute["SideEffect"]
//  - mark every node as "dead"
//  - erase marks on everything leading to a side-effect or an output
//  - erase everything still marked as "dead"

class DeadCodeEliminator : public Propagator {
 public:
  set<Field*, CompilationElement_cmp> kill_f; set<AM*> kill_a;
  DeadCodeEliminator(ProtoAnalyzer* parent, Args* args)
    : Propagator(true,false,true) {
    verbosity = args->extract_switch("--dead-code-eliminator-verbosity") ? 
      args->pop_number() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "DeadCodeEliminator"; }
  void preprop() { kill_f = worklist_f; kill_a = worklist_a; }
  void postprop() {
    any_changes = !kill_f.empty() || !kill_a.empty();
    while(!kill_f.empty()) {
      Field* f = *kill_f.begin(); kill_f.erase(f);
      if(verbosity>=2) *cpout<<" Deleting field "<<f->to_str()<<endl;
      f->container->delete_node(f->producer);
    }
    while(!kill_a.empty()) {
      AM* am = *kill_a.begin(); kill_a.erase(am);
      if(verbosity>=2) *cpout<<" Deleting AM "<<am->to_str()<<endl;
      am->container->delete_space(am);
    }
  }
  
  void act(Field* f) {
    if(!kill_f.count(f)) return; // only check the dead
    bool live=false;
    if(f->container->output==f) live=true; // output is live
    if(f->producer->op->attributes.count(":side-effect")) live=true;
    set<pair<OperatorInstance*,int> >::iterator i=f->consumers.begin();
    for( ;i!=f->consumers.end();i++) // live consumer -> live
      if(!kill_f.count((*i).first->output)) {live=true;break;}
    for(int i=0;i<f->selectors.size();i++) // live selector -> live
      if(!kill_a.count(f->selectors[i])) {live=true;break;}
    if(live) { kill_f.erase(f); note_change(f); }
  }
  
  void act(AM* am) {
    if(!kill_a.count(am)) return; // only check the dead
    bool live=false;
    for(set<AM*>::iterator i=am->children.begin();i!=am->children.end();i++)
      if(!kill_a.count(*i)) {live=true;break;} // live child -> live
    for(set<Field*>::iterator i=am->fields.begin();i!=am->fields.end();i++)
      if(!kill_f.count(*i)) {live=true;break;} // live domain -> live
    if(live) { kill_a.erase(am); note_change(am); }
  }
};


/*****************************************************************************
 *  INLINING                                                                 *
 *****************************************************************************/

#define DEFAULT_INLINING_THRESHOLD 10
class FunctionInlining : public Propagator {
 public:
  int threshold; // # ops to make something an inlining target
  set<OperatorInstance*> targets;
  FunctionInlining(ProtoAnalyzer* parent, Args* args)
    : Propagator(false,true,false) {
    verbosity = args->extract_switch("--function-inlining-verbosity") ? 
      args->pop_number() : parent->verbosity;
    threshold = args->extract_switch("--function-inlining-threshold") ?
      args->pop_number() : DEFAULT_INLINING_THRESHOLD;
  }
  virtual void print(ostream* out=0) { *out << "FunctionInlining"; }
  
  void act(OperatorInstance* oi) {
    if(!oi->op->isA("CompoundOp")) return;
    if(threshold!=-1 &&
       ((CompoundOp*)oi->op)->body->nodes.size() > threshold) return;
    // online inline small compound operators
    if(verbosity>=2) *cpout<<" Inlining function "<<oi->to_str()<<endl;
    // TODO: make sure ths is safe to do for nested operators
    root->make_op_inline(oi);
  }
};


/*****************************************************************************
 *  INTEGRITY CERTIFICATION                                                  *
 *****************************************************************************/

class CertifyBackpointers : public Propagator {
 public:
  bool bad;
  CertifyBackpointers(int verbosity):Propagator(true,true,true){
    this->verbosity = verbosity;
  }
  virtual void print(ostream* out=0) { *out << "CertifyBackpointers"; }
  void preprop() { bad=false; }
  void postprop() {
    if(bad) ierror("Backpointer certification failed.");
  }
  void act(Field* f) {
    // producer OK
    if(! (f->container->nodes.count(f->producer) && f->producer->output==f))
      { bad=true; compile_error(f,"Bad producer of "+f->to_str()); }
    // consumer OK
    set<pair<OperatorInstance*,int> >::iterator i=f->consumers.begin();
    for( ;i!=f->consumers.end();i++) {
      if(! (f->container->nodes.count((*i).first) &&
            (*i).second < (*i).first->inputs.size() &&
            (*i).first->inputs[(*i).second]==f))
        { bad=true; compile_error(f,"Bad consumer "+i2s((*i).second)+" of "+f->to_str());}
    }
    // selectors OK
    for(int i=0;i<f->selectors.size();i++) {
      if(! (f->container->spaces.count(f->selectors[i]) &&
            f->selectors[i]->selector==f))
        { bad=true; compile_error(f,"Bad selector "+i2s(i)+" of "+f->to_str());}
    }
    // domain OK
    if(!(f->container->spaces.count(f->domain) && f->domain->fields.count(f)))
      { bad=true; compile_error(f,"Bad domain of "+f->to_str()); }
    // container OK
    if(!f->container->edges.count(f))
      { bad=true; compile_error(f,"Bad container of "+f->to_str()); }
  }
  
  void act(OperatorInstance* oi) {
    // output OK
    if(! (oi->container->edges.count(oi->output) && oi->output->producer==oi))
      { bad=true; compile_error(oi,"Bad output of "+oi->op->to_str()); }
    // inputs OK
    for(int i=0;i<oi->inputs.size();i++) {
      if(!oi->container->edges.count(oi->inputs[i]) ||
         !oi->inputs[i]->consumers.count(make_pair(oi,i)))
       {bad=true;compile_error(oi,"Bad input "+i2s(i)+" of "+oi->op->to_str());}
    }
    // container OK
    if(!oi->container->nodes.count(oi))
      { bad=true; compile_error(oi,"Bad container of "+oi->op->to_str()); }
  }
  
  void act(AM* am) {
    // selector & parent OK
    if(am->parent) {
      if(!am->container->edges.count(am->selector) ||
         index_of(&am->selector->selectors,am)==-1)
        {bad=true;compile_error(am,"Bad selector of "+am->to_str()); }
      if(!am->container->spaces.count(am->parent) ||
         !am->parent->children.count(am))
        {bad=true;compile_error(am,"Bad parent of "+am->to_str()); }
    }
    // children OK
    for(set<AM*>::iterator i=am->children.begin();i!=am->children.end();i++)
      if(!(am->container->spaces.count(*i) && (*i)->parent==am)) 
        { bad=true; compile_error(am,"Bad child of "+am->to_str()); }
    // fields OK
    for(set<Field*>::iterator i=am->fields.begin();i!=am->fields.end();i++)
      if(!(am->container->edges.count(*i) && (*i)->domain==am))
        { bad=true; compile_error(am,"Bad field of "+am->to_str()); }
    // container OK
    if(!am->container->spaces.count(am))
      { bad=true; compile_error(am,"Bad container of "+am->to_str()); }
  }
};

/*****************************************************************************
 *  TOP-LEVEL ANALYZER                                                       *
 *****************************************************************************/

// some test classes
class InfiniteLoopPropagator : public Propagator {
public:
  InfiniteLoopPropagator() : Propagator(true,false) {}
  void act(Field* f) { note_change(f); }
};
class NOPPropagator : public Propagator {
public:
  NOPPropagator() : Propagator(true,true,true) {}
};


ProtoAnalyzer::ProtoAnalyzer(NeoCompiler* parent, Args* args) {
  is_dump_analyzed = args->extract_switch("-CDanalyzed") | parent->is_dump_all;
  verbosity = args->extract_switch("--analyzer-verbosity")?args->pop_number():0;
  max_loops=args->extract_switch("--analyzer-max-loops")?args->pop_number():10;
  paranoid = args->extract_switch("--analyzer-paranoid");
  // set up rule collection
  rules.push_back(new TypePropagator(this,args));
  rules.push_back(new ConstantFolder(this,args));
  rules.push_back(new Literalizer(this,args));
  rules.push_back(new DeadCodeEliminator(this,args));
}

void ProtoAnalyzer::analyze(DFG* g) {
  CertifyBackpointers checker(verbosity);
  if(paranoid) checker.propagate(g); // make sure we're starting OK
  for(int i=0;i<max_loops;i++) {
    if(!g->edges.size() || !g->nodes.size()) {
      compile_error("Program has no content."); terminate_on_error();
    }
    bool changed=false;
    for(int j=0;j<rules.size();j++) {
      changed |= rules[j]->propagate(g); terminate_on_error();
      if(paranoid) checker.propagate(g); // make sure we didn't break anything
    }
    if(!changed) break;
    if(i==(max_loops-1))
      compile_warn("Analyzer giving up after "+i2s(max_loops)+" loops");
  }
  checker.propagate(g); // make sure we didn't break anything
  if(is_dump_analyzed) g->print_with_funcalls(cpout);
}


/*****************************************************************************
 *  GLOBAL-TO-LOCAL TRANSFORMER                                              *
 *****************************************************************************/

Operator* op_err(CompilationElement *where,string msg); // from interpreter

// Changes neighborhood operations to fold-hoods
// Mechanism:
// 1. find the summary operation
// 2. get the tree of field ops leading to it and turn them into a compound op
// 3. inputs to nbr ops are combined together into a tuple input
// 4. inputs to locals are turned are marked with "loopref"
// 5. 
class HoodToFolder : public Propagator {
public:
  HoodToFolder(GlobalToLocal* parent, Args* args) : Propagator(false,true) {
    verbosity = args->extract_switch("--hood-to-folder-verbosity") ?
      args->pop_number() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "HoodTofolder"; }

  Field* make_nbr_subroutine(OperatorInstance* oi,vector<Field*>* locals,
                             vector<Field>* exports) {
  }

  /*
  Operator* nbr_op_to_folder(OperatorInstance* oi) {
    string name = oi->op->name;
    if(name=="min-hood") { return min;
    } else if(name=="max-hood") { return max;
    } else if(name=="any-hood") { return or;
    } else if(name=="all-hood") { return and;
    } else if(name=="int-hood") { return +;
    } else {
      return op_err(oi,"Can't convert summary '"+name+"' to local operator");
    }
  }

  void act(OperatorInstance* oi) {
    // conversions begin at summaries (field-to-local ops)
    if(oi->inputs.size()==1 && oi->inputs[0]->range->isA("ProtoField") &&
       oi->output->range->isA("ProtoLocal")) {
      // (fold-hood folder init input-tuple)
      vector<Field*> locals, exports;
      Field* nbrop = make_nbr_subroutine(oi,&locals, &exports);
      if(oi->op->name=="int-hood") {
        add a multiplication by (infinitesimal) to the nbrop output;
      }
      Operator* folder = nbr_op_to_folder(oi->op);
      OperatorInstance* noi = new OperatorInstance(fold_hood_plus,oi->output->domain);
      oi->parent->inherit_and_add(noi,oi);
      // first, the folding routine
      OperatorInstance* fop = new OperatorInstance(folder.first,oi->output->domain);
      oi->parent->inherit_and_add(fop,oi);
      noi->inputs.push_back(fop); // folder op
      // next, the fusing routing
      Literal* lit = new Literal(folder.second);
      OperatorInstance* iop = new OperatorInstance(lit,oi->output->domain);
      oi->parent->inherit_and_add(iop,oi);
      noi->inputs.push_back(iop); // init op
      if(exports.size()==1) {
        noi->inputs.push_back(exports[0]);
      } else {
        OperatorInstance* tup = new OperatorInstance(tup,oi->output->domain);
        oi->parent->inherit_and_add(tup,oi);
        noi->inputs.push_back(tup->output);
        for(int i=0;i<exports.size();i++) tup->inputs.push_back(exports[i]);
      }
      // get the appropriate operator
      // 
    }
  }

  */
};

GlobalToLocal::GlobalToLocal(NeoCompiler* parent, Args* args) {
  is_dump_localized = args->extract_switch("-CDlocalized")|parent->is_dump_all;
  verbosity=args->extract_switch("--localizer-verbosity")?args->pop_number():0;
  max_loops=args->extract_switch("--localizer-max-loops")?args->pop_number():10;
  paranoid = args->extract_switch("--localizer-paranoid");
  // set up rule collection
  rules.push_back(new HoodToFolder(this,args));
}

void GlobalToLocal::localize(DFG* g) {
  CertifyBackpointers checker(verbosity);
  if(paranoid) checker.propagate(g); // make sure we're starting OK
  for(int i=0;i<max_loops;i++) {
    if(!g->edges.size() || !g->nodes.size()) {
      compile_error("Program has no content."); terminate_on_error();
    }
    bool changed=false;
    for(int j=0;j<rules.size();j++) {
      changed |= rules[j]->propagate(g); terminate_on_error();
      if(paranoid) checker.propagate(g); // make sure we didn't break anything
    }
    if(!changed) break;
    if(i==(max_loops-1))
      compile_warn("Localizer giving up after "+i2s(max_loops)+" loops");
  }
  checker.propagate(g); // make sure we didn't break anything
  if(is_dump_localized) g->print_with_funcalls(cpout);
}

/* GOING GLOBAL TO LOCAL:
   Three transformations: restriction, feedback, neighborhood

   Hood: key question - how far should local computations reach?
     Proposal: all no-input-ops are stuck into a let via "local" op
     So... can identify subgraph that bounds inputs...
       put that subgraph into a new compound - second LAMBDA
       put all NBR ops into a tuple, which is used for the 3rd input
       <LAMBDA>, <LAMBDA>, <LOCAL> --> [fold-hood-plus] --> <LOCAL>
       lambda of a primitive op -> FUN_4_OP, REF_1_OP, REF_0_OP, <prim>, RET_OP
     What about fields used in two different operations?
       e.g. (let ((v (nbr x))) (- (min-hood v) (max-hood v)))
       that looks like two different exports...
 */

/*
  Current failure rate: 17/69

  First thing to build:
    get the damned type reasoning right!  DONE
    FieldOp -> pointwise "pushed down" to field
    find_hood_fn_ops
    ops_to_hood_fn
 */
