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
      ProtoTuple* tp = T_TYPE(t);
      for(int i=0;i<tp->types.size();i++)
        if(!acceptable(tp->types[i])) return false;
      return true;
    } else if(t->isA("ProtoLambda")) {
      return L_VAL(t)==NULL || acceptable(L_VAL(t));
    } else if(t->isA("ProtoField")) {
      return F_VAL(t)==NULL || acceptable(F_VAL(t));
    } else return false; // all others
  }
  // concreteness of operators, for ProtoLambda:
  static bool acceptable(Operator* op) { 
    if(op->isA("Literal") || op->isA("Parameter") || op->isA("Primitive")) {
      return true;
    } else if(op->isA("CompoundOp")) {
      return true; // its fields are checked elsewhere
    } else return false; // generic operator
  }
};

void CheckTypeConcreteness::act(Field* f) {
  if(!Concreteness::acceptable(f->range))
    //ierror(f,"Type is ambiguous: "+f->to_str());
    compile_error(f,"Type is ambiguous: "+f->to_str());
}

/*****************************************************************************
 *  TYPE CONSTRAINTS                                                         *
 *****************************************************************************/

ProtoType* ref_err_scratch = new ProtoType();
ProtoType** typeref_error(CompilationElement *where,string msg) {
  compile_error(where,"Type reference: "+msg); return &ref_err_scratch;
}

SExpr* get_sexp(CE* src, string attribute) {
  Attribute* a = src->attributes[attribute];
  if(a==NULL || !a->isA("SExprAttribute"))
    return sexp_err(src,"Couldn't get expression for: "+a->to_str());
  return ((SExprAttribute*)a)->exp;
}


ProtoType** get_type_ref(OperatorInstance* oi, SExpr* ref) {
  if(ref->isSymbol()) {
    if(*ref=="value") {
      return &oi->output->range;
    }
  } else if(ref->isList()) {
    SE_List_iter li(ref);
    if(li.on_token("nth")) {
      // Challenge: with an unknown ref, this can apply to all the elements!
      // Dilemma: do we go with "return a reference to all" or do we make
      // parallel get & set operations?  I'm inclined to the latter...
    }
  }
  return typeref_error(ref,"Unknown type reference: "+ce2s(ref));
}

// new system: :type-constraints SExprs
// assumes constraint has already been checked for syntactic sanity
void apply_constraint(OperatorInstance* oi, SE_List* constraint) {
  SE_List_iter li(constraint);
  if(li.on_token("=")) { // types should be identical
    ProtoType **a = get_type_ref(oi,li.get_next("type reference"));
    ProtoType **b = get_type_ref(oi,li.get_next("type reference"));
    ProtoType *joint = ProtoType::gcs(*a,*b); 
    if(joint==NULL) {
      ierror("Equality constraint failed, but don't know how to handle yet.");
    } else {
      cout << "Action "<<ce2s(constraint)<<" should assert "<<ce2s(joint)<<endl;
      //maybe_set_ref(oi,constraint[0],joint);
      //maybe_set_ref(oi,constraint[1],joint);
    }
    // first, take the GCS, then compare them together
    // if it's OK, then push back:  maybe_set_ref(oi,constraint[i]);
    // if it's not OK, then call for resolution
  } else {
    compile_error("Unknown constraint '"+ce2s(li.peek_next())+"'");
  }
}

void apply_constraints(OperatorInstance* oi, SExpr* constraints) {
  if(!constraints->isList()) 
    {compile_error(constraints,"Constraints must be list: "+ce2s(constraints));return;}
  SE_List_iter li((SE_List*)constraints);
  while(li.has_next()) {
    SExpr* ct = li.get_next();
    if(!ct->isList() || ((SE_List*)ct)->len()!=3)
      {compile_error(ct,"Constraint must be a 3-item list: "+ce2s(ct)); return;}
    apply_constraint(oi,(SE_List*)ct);
  }
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
      ProtoTuple* tp = T_TYPE(t);
      for(int i=0;i<tp->types.size();i++)
        if(!acceptable(tp->types[i])) return false;
      return true;
    } else if(t->isA("ProtoLambda")) {
      Signature* s = L_VAL(t)->signature;
      if(!acceptable(s->output)) return false;
      if(s->rest_input && !acceptable(s->output)) return false;
      for(int i=0;i<s->required_inputs.size();i++)
        if(!acceptable(s->required_inputs[i])) return false;
      for(int i=0;i<s->optional_inputs.size();i++)
        if(!acceptable(s->optional_inputs[i])) return false;
      return true;
    } else if(t->isA("ProtoField")) {
      return F_VAL(t)==NULL || acceptable(F_VAL(t));
    } else return false; // all others
  }
};



// used during type resolution
struct TypeVector : public ProtoTuple { reflection_sub(TypeVector,ProtoTuple);
  //vector<ProtoType*> types;
  TypeVector() : ProtoTuple(true) {}
  virtual void print(ostream* out=0);
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
  TypeVector *tv = new TypeVector();
  for(int i=0;i<sig->required_inputs.size();i++)
    tv->types.push_back(sig->required_inputs[i]);
  for(int i=0;i<sig->optional_inputs.size();i++)
    tv->types.push_back(sig->optional_inputs[i]);
  if(sig->rest_input!=NULL) {
    tv->bounded = false;tv->types.push_back(sig->rest_input);
  }
  return tv;
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
  if(!TypeResolution::acceptable(t)) return NULL;
  // don't derive from signature type mismatches
  if(TypeResolution::acceptable(sigtype) && !sigtype->supertype_of(t)) 
    return NULL;
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

ProtoType* OperatorInstance::output_type() {
  return if_derivable(output->range, op->signature->output);
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
  else if (s->name.substr(0,3)=="arg") {
    int n = atoi(s->name.substr(3,s->name.size()-3).c_str());
    return oi->nth_input(n);
  }
  // must be value
  return oi->output_type();
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
    if(DerivedType::is_arg_ref(var)) { 
      return arg_ref((SE_Symbol*)ref,oi);
    } else if(var=="return") {
      if(!oi->op->isA("CompoundOp")) 
        return type_error(ref,"'return' only valid for compound operators");
      ProtoType* rtype = ((CompoundOp*)oi->op)->output->range;
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
      vector<ProtoType*> *source; bool bounded;
      if(subs[0]->isA("TypeVector")) {
        source = &(dynamic_cast<TypeVector*>(subs[0]))->types;
        bounded = (dynamic_cast<TypeVector*>(subs[0]))->bounded;
      } else if(subs[0]->isA("ProtoTuple")) {
        source = &(dynamic_cast<ProtoTuple*>(subs[0]))->types;
        bounded = (dynamic_cast<ProtoTuple*>(subs[0]))->bounded;
      } else return type_error(oi,"'nth' needs tuple: "+subs[0]->to_str());
      if(source->size()==0) return type_error(oi,"'nth' source is empty");
      if(n->constant) { // if 2nd is a literal, access the argument
        if(n->value<0 || (bounded && n->value>=source->size()))
          return type_error(oi,"'nth' reference out of bounds: "+n->to_str()+" on "+subs[0]->to_str());
        return (*source)[MIN((int)n->value,source->size()-1)];
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
        rtype = ((CompoundOp*)t->op)->output->range;
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
        rtype = TypeVector::input_types(t->op->signature);
        //ierror("input derivation not yet done for compoundops");
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

class TypePropagator : public IRPropagator {
 public:
  TypePropagator(DFGTransformer* parent, Args* args) : IRPropagator(true,true) {
    verbosity = args->extract_switch("--type-propagator-verbosity") ? 
      args->pop_int() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "TypePropagator"; }
  
  // apply a consumer constraint, managing implicit type conversions 
  bool back_constraint(ProtoType** tmp,Field* f,pair<OperatorInstance*,int> c) {
    ProtoType* rawc = c.first->op->signature->nth_type(c.second);
    ProtoType* ct = resolve_type(c.first,rawc);
    if(!ct) return true;// ignore unresolved
    ProtoType* newtype = ProtoType::gcs(*tmp,ct);
    V3<<"   Back constraint on: "<<ce2s(c.first)<<
      "\n    "<<ce2s(*tmp)<<" vs. "<<ce2s(ct)<<"...";
    if(newtype) { V3<<" ok\n"; *tmp = newtype; return true; }
    else V3<<" FAIL\n";

    // On merge failure, either add an implicit type conversion or error
    // if vector is needed and scalar is provided, convert to a size-1 vector
    if((*tmp)->isA("ProtoScalar") && ct->isA("ProtoVector")) {
      V2<<"Converting Scalar to 1-Vector\n";
      OI* tup = new OperatorInstance(f->producer,Env::core_op("tup"),f->domain);
      tup->add_input(f);
      root->relocate_source(c.first,c.second,tup->output);
      note_change(tup); return false; // let constraint retry later...
    }

    // if source is local and user wants a field, add a "local" op
    // or replace no-argument source with a field op
    if((*tmp)->isA("ProtoLocal") && ct->isA("ProtoField")) {
      ProtoField* ft = dynamic_cast<ProtoField*>(ct);

      if(!ft->hoodtype || ProtoType::gcs(*tmp,ft->hoodtype)) {
        Operator* fo = FieldOp::get_field_op(f->producer);
        if(f->producer->inputs.size()==0 && f->producer->pointwise()!=0 && fo) {
          V2<<"  Fieldify pointwise: "<<ce2s(f->producer)<<endl;
          OI* foi = new OperatorInstance(f->producer,fo,f->domain);
          root->relocate_source(c.first,c.second,foi->output);
          note_change(foi); return false; // let constraint retry later...
        } else {
          V2<<"  Inserting 'local' at "<<ce2s(f)<<endl;
          OI* local = new OI(f->producer,Env::core_op("local"),f->domain);
          local->add_input(f);
          root->relocate_source(c.first,c.second,local->output);
          note_change(local); return false; // let constraint retry later...
        }
      }
    }
    
    // if source is field and user is "local", send to the local's consumers
    if((*tmp)->isA("ProtoField") && c.first->op==Env::core_op("local")) {
      V2<<"  Deleting 'local' at "<<ce2s(f)<<endl;
      root->relocate_consumers(c.first->output,f);
      return false;
    }
    
    // if source is field and user is pointwise, upgrade to field op
    if((*tmp)->isA("ProtoField") && ct->isA("ProtoLocal")) {
      ProtoField* ft = dynamic_cast<ProtoField*>(*tmp);
      int pw = c.first->pointwise();
      if(pw!=0 && (!ft->hoodtype || ProtoType::gcs(ct,ft->hoodtype))) {
        Operator* fo = FieldOp::get_field_op(c.first); // might be upgradable
        if(fo) {
          V2<<"  Fieldify pointwise: "<<ce2s(c.first)<<endl;
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
    V3 << " Considering field "<<ce2s(f)<<endl;
    // Ignore old type (it may change) [except Parameter]; use producer type
    ProtoType* tmp=resolve_type(f->producer,f->producer->op->signature->output);
    if(f->producer->op->isA("Parameter")) tmp=f->range;
    if(!tmp) return; // if producer unresolvable, give up
    // GCS against consumers
    for_set(Consumer,f->consumers,i)
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
    V3 << " Considering op instance "<<ce2s(oi)<<endl;
     //check if number of inputs is legal
    if(!oi->op->signature->legal_length(oi->inputs.size())) {
      compile_error(oi,oi->to_str()+" has "+i2s(oi->inputs.size())+" argumen"+
                    "ts, but signature is "+oi->op->signature->to_str());
      return;
    }
    
    if(oi->op->isA("Primitive")) { // constrain against signature
      // new-style resolution
      if(oi->op->marked(":type-constraints"))
        apply_constraints(oi,get_sexp(oi->op,":type-constraints"));
      
      // old-style resolution
      
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
      OIset *srcs = &root->funcalls[((Parameter*)oi->op)->container];
      for_set(OI*,*srcs,i) {
        if((*i)->op->isA("Literal")) return; // can't work with lambdas
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
      ProtoType* rtype = ((CompoundOp*)oi->op)->output->range;
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
    return (S_VAL(a)==S_VAL(b)) ? 0 : ((S_VAL(a)>S_VAL(b)) ? 1 : -1);
  } else if(a->isA("ProtoScalar") || b->isA("ProtoScalar")) {
    int sx = a->isA("ProtoScalar") ? -1 : 1;
    double s = S_VAL(a->isA("ProtoScalar")?a:b);
    ProtoTuple* v = T_TYPE(a->isA("ProtoScalar")?b:a);
    if(s==S_VAL(v->types[0])) {
      for(int i=1;i<v->types.size();i++) {
        if(S_VAL(v->types[i])!=0) return sx*((S_VAL(v->types[1])>0)?1:-1);
      }
      return 0;
    } else return (sx * ((S_VAL(v->types[0]) > s) ? 1 : -1));
  } else { // 2 vectors
    ProtoTuple *va = T_TYPE(a), *vb = T_TYPE(b);
    int la = va->types.size(), lb = vb->types.size(), len = MAX(la,lb);
    for(int i=0;i<len;i++) {
      double sa=(i<la)?S_VAL(va->types[i]):0, sb=(i<lb)?S_VAL(vb->types[i]):0;
      if(sa!=sb) { return (sa>sb) ?  1 : -1; }
    }
    return 0;
  }
}

ProtoNumber* add_consts(ProtoType* a, ProtoType* b) {
  if(a->isA("ProtoScalar") && b->isA("ProtoScalar")) {
    return new ProtoScalar(S_VAL(a)+S_VAL(b));
  } else if(a->isA("ProtoScalar") || b->isA("ProtoScalar")) {
    double s = S_VAL(a->isA("ProtoScalar")?a:b);
    ProtoTuple* v = T_TYPE(a->isA("ProtoScalar")?b:a);
    ProtoVector* out = new ProtoVector(v->bounded);
    for(int i=0;i<v->types.size();i++) { 
       ProtoScalar* element = dynamic_cast<ProtoScalar*>(v->types[i]);
       out->add(new ProtoScalar(element->value));
    }
    (dynamic_cast<ProtoScalar*>(out->types[0]))->value += s;
    return out;
  } else { // 2 vectors
    ProtoTuple *va = T_TYPE(a), *vb = T_TYPE(b);
    ProtoVector* out = new ProtoVector(va->bounded && vb->bounded);
    int la = va->types.size(), lb = vb->types.size(), len = MAX(la,lb);
    for(int i=0;i<len;i++) {
      double sa=(i<la)?S_VAL(va->types[i]):0, sb=(i<lb)?S_VAL(vb->types[i]):0;
      out->add(new ProtoScalar(sa+sb));
    }
    return out;
  }
}

class ConstantFolder : public IRPropagator {
 public:
  ConstantFolder(DFGTransformer* parent, Args* args) : IRPropagator(false,true) {
    verbosity = args->extract_switch("--constant-folder-verbosity") ? 
      args->pop_int() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "ConstantFolder"; }

  // FieldOp compatible accessors
  ProtoType* nth_type(OperatorInstance* oi, int i) {
    ProtoType* src = oi->inputs[i]->range;
    if(oi->op->isA("FieldOp")) src = F_VAL(src);
    return src;
  }
  double nth_scalar(OI* oi, int i) {return S_VAL(nth_type(oi,i));}
  ProtoTuple* nth_tuple(OI* oi, int i) { return T_TYPE(nth_type(oi,i)); }
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
      if(sum->isA("ProtoScalar")) { S_VAL(sum) *= -1;
      } else { // vector
        ProtoVector* s = dynamic_cast<ProtoVector*>(sum); 
        for(int i=0;i<s->types.size();i++) S_VAL(s->types[i]) *= -1;
      }
      // add in first
      sum = add_consts(sum,nth_type(oi,0));
      maybe_set_output(oi,sum);
    } else if(name=="*") {
      double mults = 1;
      ProtoTuple* vnum = NULL;
      for(int i=0;i<oi->inputs.size();i++) {
        if(nth_type(oi,i)->isA("ProtoScalar")) { mults *= nth_scalar(oi,i);
        } else if(vnum) { compile_error(oi,">1 vector in multiplication");
        } else { vnum = nth_tuple(oi,i); 
        }
      }
      // final optional stage of vector multiplication
      if(vnum) {
        ProtoVector *pv = new ProtoVector(vnum->bounded);
        for(int i=0;i<vnum->types.size();i++)
          pv->add(new ProtoScalar(S_VAL(vnum->types[i])*mults));
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
        for(int i=0;i<num->types.size();i++)
          pv->add(new ProtoScalar(S_VAL(num->types[i])/denom));
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
      // check two, equal length vectors
      ProtoVector* v1 = NULL;
      ProtoVector* v2 = NULL;
      if(2==oi->inputs.size() 
         && nth_type(oi,0)->isA("ProtoVector") 
         && nth_type(oi,1)->isA("ProtoVector")) {
        v1 = dynamic_cast<ProtoVector*>(nth_type(oi,0)); 
        v2 = dynamic_cast<ProtoVector*>(nth_type(oi,1)); 
        if(v1->types.size() != v2->types.size()) {
          compile_error("Dot product requires 2, *equal size* vectors");
        }
      } else {
        compile_error("Dot product requires exactly *2* vectors");
      }
      // sum of products
      ProtoScalar* sum = new ProtoScalar(0);
      for(int i=0; i<v1->types.size(); i++) {
        sum->value += dynamic_cast<ProtoScalar*>(v1->types[i])->value *
          dynamic_cast<ProtoScalar*>(v2->types[i])->value;
      }
      maybe_set_output(oi,sum);
    } else if(name=="min-hood" || name=="max-hood" || name=="any-hood"
              || name=="all-hood") {
      maybe_set_output(oi,F_VAL(oi->inputs[0]->range));
    }
  }
  
};

/*****************************************************************************
 *  LITERALIZER                                                              *
 *****************************************************************************/

class Literalizer : public IRPropagator {
 public:
  Literalizer(DFGTransformer* parent, Args* args) : IRPropagator(true,true) {
    verbosity = args->extract_switch("--literalizer-verbosity") ? 
      args->pop_int() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "Literalizer"; }
  void act(Field* f) {
    if(f->producer->op->isA("Literal")) return; // literals are already set
    if(f->producer->op->attributes.count(":side-effect")) return; // keep sides
    if(f->range->isLiteral()) {
      OI *oldoi = f->producer;
      OI *newoi = root->add_literal(f->range,f->domain,oldoi)->producer;
      V2<<" Literalizing:\n  "<<ce2s(oldoi)<<" \n  into "<<ce2s(newoi)<<endl;
      root->relocate_consumers(f,newoi->output); note_change(f);
      root->delete_node(oldoi); // kill old op
    }
  }

  void act(OperatorInstance* oi) {
    if(!oi->op->isA("Primitive")) return; // only operates on primitives
    string name = ((Primitive*)oi->op)->name;
    // change "apply" of literal lambda into just an OI of that operator
    if(name=="apply") {
      if(oi->inputs[0]->range->isA("ProtoLambda") && 
         oi->inputs[0]->range->isLiteral()) {
        OI* newoi=new OI(oi,L_VAL(oi->inputs[0]->range),oi->output->domain);
        for(int i=1;i<oi->inputs.size();i++) newoi->add_input(oi->inputs[i]);
        V2<<" Literalizing:\n  "<<ce2s(oi)<<" \n  into "<<ce2s(newoi)<<endl;
        root->relocate_consumers(oi->output,newoi->output); note_change(oi);
        root->delete_node(oi); // kill old op
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

map<string,pair<bool,int> > arg_mem;
int mem_arg(Args* args,string name,int defaultv) {
  if(!arg_mem.count(name)) {
    if(args->extract_switch(name.c_str()))
      arg_mem[name] = make_pair<bool,int>(true,args->pop_int());
    else
      arg_mem[name] = make_pair<bool,int>(false,-1);
  }
  return arg_mem[name].first ? arg_mem[name].second : defaultv;
}

class DeadCodeEliminator : public IRPropagator {
 public:
  Fset kill_f; AMset kill_a;
  DeadCodeEliminator(DFGTransformer* par, Args* args)
    : IRPropagator(true,false,true) {
    verbosity=mem_arg(args,"--dead-code-eliminator-verbosity",par->verbosity);
  }
  
  virtual void print(ostream* out=0) { *out << "DeadCodeEliminator"; }
  void preprop() { kill_f = worklist_f; kill_a = worklist_a; }
  void postprop() {
    any_changes = !kill_f.empty() || !kill_a.empty();
    while(!kill_f.empty()) {
      Field* f = *kill_f.begin(); kill_f.erase(f);
      V2<<" Deleting field "<<ce2s(f)<<endl;
      root->delete_node(f->producer);
    }
    while(!kill_a.empty()) {
      AM* am = *kill_a.begin(); kill_a.erase(am);
      V2<<" Deleting AM "<<ce2s(am)<<endl;
      root->delete_space(am);
    }
  }
  
  void act(Field* f) {
    if(!kill_f.count(f)) return; // only check the dead
    bool live=false;
    if(f->is_output()) live=true; // output is live
    if(f->producer->op->attributes.count(":side-effect")) live=true;
    for_set(Consumer,f->consumers,i) // live consumer -> live
      if(!kill_f.count((*i).first->output)) {live=true;break;}
    for_set(AM*,f->selectors,ai) // live selector -> live
      if(!kill_a.count(*ai)) {live=true;break;}
    if(live) { kill_f.erase(f); note_change(f); }
  }
  
  void act(AM* am) {
    if(!kill_a.count(am)) return; // only check the dead
    bool live=false;
    for_set(AM*,am->children,i)
      if(!kill_a.count(*i)) {live=true;break;} // live child -> live
    for_set(Field*,am->fields,i)
      if(!kill_f.count(*i)) {live=true;break;} // live domain -> live
    if(am->bodyOf!=NULL) {live=true;} // don't delete root AMs
    if(live) { kill_a.erase(am); note_change(am); }
  }
};


/*****************************************************************************
 *  INLINING                                                                 *
 *****************************************************************************/

#define DEFAULT_INLINING_THRESHOLD 10
class FunctionInlining : public IRPropagator {
 public:
  int threshold; // # ops to make something an inlining target
  FunctionInlining(DFGTransformer* parent, Args* args)
    : IRPropagator(false,true,false) {
    verbosity = args->extract_switch("--function-inlining-verbosity") ? 
      args->pop_int() : parent->verbosity;
    threshold = args->extract_switch("--function-inlining-threshold") ?
      args->pop_int() : DEFAULT_INLINING_THRESHOLD;
  }
  virtual void print(ostream* out=0) { *out << "FunctionInlining"; }
  
  void act(OperatorInstance* oi) {
    if(!oi->op->isA("CompoundOp")) return; // can only inline compound ops
    if(oi->recursive()) return; // don't inline recursive
    // check that either the body or the container is small
    Fset bodyfields;
    int bodysize = ((CompoundOp*)oi->op)->body->size();
    int containersize = oi->output->domain->root()->size();
    if(threshold!=-1 && bodysize>threshold && containersize>threshold) return;
    
    // actually carry out the inlining
    V2<<" Inlining function "<<ce2s(oi)<<endl;
    note_change(oi); root->make_op_inline(oi);
  }
};


/*****************************************************************************
 *  TOP-LEVEL ANALYZER                                                       *
 *****************************************************************************/

// some test classes
class InfiniteLoopPropagator : public IRPropagator {
public:
  InfiniteLoopPropagator() : IRPropagator(true,false) {}
  void act(Field* f) { note_change(f); }
};
class NOPPropagator : public IRPropagator {
public:
  NOPPropagator() : IRPropagator(true,true,true) {}
};


ProtoAnalyzer::ProtoAnalyzer(NeoCompiler* parent, Args* args) {
  verbosity = args->extract_switch("--analyzer-verbosity") ? 
    args->pop_int() : parent->verbosity;
  max_loops=args->extract_switch("--analyzer-max-loops")?args->pop_int():10;
  paranoid = args->extract_switch("--analyzer-paranoid")|parent->paranoid;
  // set up rule collection
  rules.push_back(new TypePropagator(this,args));
  rules.push_back(new ConstantFolder(this,args));
  rules.push_back(new Literalizer(this,args));
  rules.push_back(new DeadCodeEliminator(this,args));
  rules.push_back(new FunctionInlining(this,args));
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
class HoodToFolder : public IRPropagator {
public:
  HoodToFolder(GlobalToLocal* parent, Args* args) : IRPropagator(false,true) {
    verbosity = args->extract_switch("--hood-to-folder-verbosity") ?
      args->pop_int() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "HoodToFolder"; }
  
  CEmap(Operator*,CompoundOp*) localization_cache;
  Operator* localize_operator(Operator* op) {
    V4<<"    Localizing operator: "<<ce2s(op)<<endl;
    if(op->isA("Literal")) {
      if(((Literal*)op)->value->isA("ProtoField"))
        return new Literal(op,F_VAL(((Literal*)op)->value)); // strip field
      else return op;
    } else if(op->isA("Primitive")) {
      Operator* local = LocalFieldOp::get_local_op(op);
      return (local!=NULL) ? local : op;
    } else if(op->isA("CompoundOp")) {
      V5<<"     Checking localization cache\n";
      if(localization_cache.count(op)) return localization_cache[op];
      // if it's already pointwise, just return
      V5<<"     Checking whether op is already pointwise\n";
      Fset fields; ((CompoundOp*)op)->body->all_fields(&fields);
      bool local=true;
      for_set(Field*,fields,i) {
        V5<<"      Considering field "<<ce2s(*i)<<endl;
        if((*i)->range->isA("ProtoField")) 
          local=false;
      }
      if(local) return op;
      // Walk through ops: local & nbr ops are flattened, others are localized
      V5<<"     Transforming op to local\n";
      CompoundOp* newop = new CompoundOp((CompoundOp*)op);
      OIset ois; newop->body->all_ois(&ois);
      for_set(OI*,ois,i) {
        cout<<"Attempting to localize operator: "<<ce2s(*i)<<endl;
        if((*i)->op==Env::core_op("nbr")) {
          root->relocate_consumers((*i)->output,(*i)->inputs[0]);
        } else if((*i)->op==Env::core_op("local")) {
          root->relocate_consumers((*i)->output,(*i)->inputs[0]);
        } else {
          (*i)->op = localize_operator((*i)->op);
          (*i)->output->range = (*i)->op->signature->output; // fix output
        }
      }
      localization_cache[op] = newop;
      return newop;
    } else if(op->isA("Parameter")) { 
      return op; // parameters always OK
    }
    ierror("Don't know how to localize operator: "+ce2s(op));
  }
  
  CompoundOp* make_nbr_subroutine(OperatorInstance* oi, Field** exportf) {
    V3<<"   Creating subroutine for: "<<ce2s(oi)<<endl;
    // First, find all field-valued ops feeding this summary operator
    OIset elts; vector<Field*> exports; OIset q; q.insert(oi);
    while(q.size()) {
      OperatorInstance* next = *q.begin(); q.erase(next);
      for(int i=0;i<next->inputs.size();i++) {
        Field* f = next->inputs[i];
        if(f->producer->op==Env::core_op("nbr")) { // record exported fields
          if(!f->producer->inputs.size())
            ierror("No input for nbr operator: "+f->producer->to_str());
          if(index_of(&exports,f->producer->inputs[0])==-1)
            exports.push_back(f->producer->inputs[0]);
        } 
        if(f->range->isA("ProtoField") && !elts.count(f->producer))
          { elts.insert(f->producer); q.insert(f->producer); }
      }
    }
    V3<<"    Found "<<elts.size()<<" elements"<<endl;
    V3<<"    Found "<<exports.size()<<" exports"<<endl;
    // create the compound op
    CompoundOp* cop=root->derive_op(&elts,oi->domain(),&exports,oi->inputs[0]);
    cop = (CompoundOp*)localize_operator(cop);
    //cop = tuplize_inputs(cop);
    
    // construct input structure
    if(exports.size()==0) {
      *exportf = root->add_literal(new ProtoScalar(0),oi->domain(),oi);
    } else if(exports.size()==1) {
      *exportf = exports[0];
    } else {
      OI* tup=new OperatorInstance(oi,Env::core_op("tup"),oi->domain());
      for(int i=0;i<exports.size();i++) tup->add_input(exports[i]);
      *exportf = tup->output;
    }
    vector<Field*> in; in.push_back(*exportf);
    
    // add multiplier if needed
    if(oi->op->name=="int-hood") {
      V3<<"   Adding int-hood multiplier\n";
      OI *oin=new OperatorInstance(oi,Env::core_op("infinitesimal"),cop->body);
      OI *tin=new OperatorInstance(oi,Env::core_op("*"),cop->body);
      tin->add_input(oin->output); tin->add_input(cop->output);
      cop->output = tin->output;
    }
    return cop;
  }

  Operator* nbr_op_to_folder(OperatorInstance* oi) {
    string name = oi->op->name;
    V3<<"   Selecting folder for: "<<name<<endl;
    if(name=="min-hood") { return Env::core_op("min");
    } else if(name=="max-hood") { return Env::core_op("max");
    } else if(name=="any-hood") { return Env::core_op("max");
    } else if(name=="all-hood") { return Env::core_op("min");
    } else if(name=="int-hood") { return Env::core_op("+");
    } else if(name=="sum-hood") { return Env::core_op("+");
    } else {
      return op_err(oi,"Can't convert summary '"+name+"' to local operator");
    }
  }

  void act(OperatorInstance* oi) {
    // conversions begin at summaries (field-to-local ops)
    if(oi->inputs.size()==1 && oi->inputs[0]->range->isA("ProtoField") &&
       oi->output->range->isA("ProtoLocal")) {
      V2<<"  Changing to fold-hood: "<<ce2s(oi)<<endl;
      // (fold-hood-plus folder fn input)
      AM* space = oi->output->domain; Field* exportf;
      CompoundOp* nbrop = make_nbr_subroutine(oi,&exportf);
      Operator* folder = nbr_op_to_folder(oi);
      OI* noi = new OperatorInstance(oi,Env::core_op("fold-hood-plus"),space);
      // hook the inputs up to the fold-hood
      V3<<"   Connecting inputs to foldhood\n";
      noi->add_input(root->add_literal(new ProtoLambda(folder),space,oi));
      noi->add_input(root->add_literal(new ProtoLambda(nbrop),space,oi));
      noi->add_input(exportf);
      // switch consumers and quit
      V3<<"   Changing over consumers\n";
      root->relocate_consumers(oi->output,noi->output); note_change(oi);
      root->delete_node(oi);
    }
  }
};

GlobalToLocal::GlobalToLocal(NeoCompiler* parent, Args* args) {
  verbosity=args->extract_switch("--localizer-verbosity") ? 
    args->pop_int() : parent->verbosity;
  max_loops=args->extract_switch("--localizer-max-loops")?args->pop_int():10;
  paranoid = args->extract_switch("--localizer-paranoid")|parent->paranoid;
  // set up rule collection
  rules.push_back(new HoodToFolder(this,args));
  rules.push_back(new DeadCodeEliminator(this,args));
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

/*****************************************************************************
 *  GENERIC TRANSFORMATION CYCLER                                            *
 *****************************************************************************/

void DFGTransformer::transform(DFG* g) {
  CertifyBackpointers checker(verbosity);
  if(paranoid) checker.propagate(g); // make sure we're starting OK
  for(int i=0;i<max_loops;i++) {
    bool changed=false;
    for(int j=0;j<rules.size();j++) {
      changed |= rules[j]->propagate(g); terminate_on_error();
      if(paranoid) checker.propagate(g); // make sure we didn't break anything
    }
    if(!changed) break;
    if(i==(max_loops-1))
      compile_warn("Transformer giving up after "+i2s(max_loops)+" loops");
  }
  g->determine_relevant();
  checker.propagate(g); // make sure we didn't break anything
}

