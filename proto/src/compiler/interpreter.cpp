/* Proto interpreter
Copyright (C) 2009, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// The Proto interpreter turns S-Expressions into intermediate representation

#include "config.h"
#include "nicenames.h"
#include "compiler.h"

/*****************************************************************************
 *  DUMMY ELEMENTS                                                           *
 *****************************************************************************/
// Dummies are used to fill in gaps created by syntax errors, allowing the
// compiler to find multiple errors in one pass & exit gracefully

struct DummyAM : public AM { 
  DummyAM() : AM(NULL) {}
  bool child_of(AM* am) { return true; }
};

CompilationElement* dummy(string type, CompilationElement* context) {
  CompilationElement *elt;
  if(type=="Operator") elt = new Literal(new ProtoScalar(7734));
  else if(type=="Field") {
    AM *da = new DummyAM(); da->inherit_attributes(context);
    OperatorInstance *oi = 
      new OperatorInstance((Operator*)dummy("Operator",context),da);
    return oi->output;
  }
  else if(type=="Type") elt = new ProtoType();
  else if(type=="Signature") elt = new Signature();
  else if(type=="Macro") elt = new Macro("*ERROR*",new SE_Symbol("*ERROR*"));
  else if(type=="SExpr") elt = new SE_Symbol("*ERROR*");
  else ierror("Don't know how to make dummy value for "+type);
  elt->inherit_attributes(context);
  elt->attributes["DUMMY"]=new MarkerAttribute(true);
  return elt;
}

Field* field_err(CompilationElement *where,string msg) {
  compile_error(where,msg); return (Field*)dummy("Field",where);
}
Operator* op_err(CompilationElement *where,string msg) {
  compile_error(where,msg); return (Operator*)dummy("Operator",where);
}
Macro* macro_err(CompilationElement *where,string msg) {
  compile_error(where,msg); return (Macro*)dummy("Macro",where);
}
ProtoType* type_err(CompilationElement *where,string msg) {
  compile_error(where,msg); return (ProtoType*)dummy("Type",where);
}
Signature* sig_err(CompilationElement *where,string msg) {
  compile_error(where,msg); return (Signature*)dummy("Signature",where);
}
SExpr* sexp_err(CompilationElement *where,string msg) {
  compile_error(where,msg); return (SExpr*)dummy("SExpr",where);
}


/*****************************************************************************
 *  ENVIRONMENTS                                                             *
 *****************************************************************************/
// specials are the tokens that are hard-wired in and thus can't be shadowed
set<string> special_tokens;
bool specials_populated = false;
void populate_specials() {
  if(specials_populated) return;
  special_tokens.insert("lambda"); special_tokens.insert("fun");
  special_tokens.insert("def"); 
  special_tokens.insert("primitive"); special_tokens.insert("macro");
  special_tokens.insert("let"); special_tokens.insert("let*"); 
  special_tokens.insert("letfed");  
  special_tokens.insert("restrict"); 
  special_tokens.insert("all");
  special_tokens.insert("include"); special_tokens.insert("tup");
  special_tokens.insert("true"); special_tokens.insert("false");
  specials_populated=true;
}
bool is_special(SExpr* s) {
  if(!s->isSymbol()) return false;
  return special_tokens.count(((SE_Symbol*)s)->name);
}

// tokens that need to be always available to the interpreter, for 
// expressions that have a syntactic component
Operator *Env::RESTRICT, *Env::DELAY, *Env::DCHANGE, *Env::NOT;
Operator *Env::MUX, *Env::ELT, *Env::LOCAL, *Env::TUP;

void Env::bind(string name, CompilationElement* value) {
  if(bindings.count(name)) 
    compile_error(value,"Cannot bind '"+name+"': already bound");
  force_bind(name,value);
}

void Env::force_bind(string name, CompilationElement* value) {
  if(special_tokens.count(name)) 
    compile_error(value,"Cannot bind '"+name+"': symbol is reserved");
  bindings[name]=value;
}

CompilationElement* Env::lookup(SE_Symbol* sym, string type) {
  CompilationElement* found = lookup(sym->name);
  if(found) {
    if(found->isA(type)) return found;
    else compile_error(sym,sym->name+" is "+found->type_of()+", not "+type);
  } else compile_error(sym,"Couldn't find definition of "+type+" "+sym->name);
  return dummy(type,sym);
}

CompilationElement* Env::lookup(string name, bool recursed) {
  if(bindings.count(name)) { return bindings[name]; // local search
  } else if(parent) { return parent->lookup(name); // search through parents
  } else if(!recursed) { // check for a file to define it
    string fname = name + ".proto";
    if(cp->parent->proto_path.find_in_path(fname)) {
      cp->interpret_file(fname);
      return lookup(name,true);
    }
  }
 return NULL;
}

/*****************************************************************************
 *  TYPES                                                                    *
 *****************************************************************************/

// SUPERTYPE RELATIONS
bool ProtoTuple::supertype_of(ProtoType* sub) { 
  if(!sub->isA(type_of())) return false; // not supertype of non-tuples
  ProtoTuple *tsub = dynamic_cast<ProtoTuple*>(sub);
  // are elements compatible?
  int ll = types.size(), sl = tsub->types.size(), len = MAX(ll,sl);
  for(int i=0;i<len;i++) {
    ProtoType *mine,*subs;
    if(i>=ll) { if(bounded) return false; else mine = types[ll-1]; }
    else mine = types[i];
    if(i>=sl) { if(tsub->bounded) return (!bounded && i==len-1); else subs = tsub->types[sl-1]; }
    else subs = tsub->types[i];
    if(!mine->supertype_of(subs)) return false;
  }
  return true;
}
bool ProtoSymbol::supertype_of(ProtoType* sub) { 
  if(!sub->isA("ProtoSymbol")) return false; // not supertype of non-symbols
  ProtoSymbol *ssub = dynamic_cast<ProtoSymbol*>(sub);
  return !constant || value==ssub->value;
}

bool ProtoScalar::supertype_of(ProtoType* sub) { 
  if(!sub->isA(type_of())) return false; // not supertype of non-scalars
  ProtoScalar *ssub = dynamic_cast<ProtoScalar*>(sub);
  return !constant || value==ssub->value || (isnan(value)&&isnan(ssub->value));
}

bool ProtoLambda::supertype_of(ProtoType* sub) { 
  if(!sub->isA(type_of())) return false; // not supertype of non-lambdas
  ProtoLambda* lsub = dynamic_cast<ProtoLambda*>(sub);
  // generic is super of all fields
  if(!op) return true; if(!lsub->op) return false;
  return op==lsub->op; // lambdas are the same only if they have the same op
}

bool ProtoField::supertype_of(ProtoType* sub) { 
  if(!sub->isA(type_of())) return false; // not supertype of non-fields
  ProtoField *fsub = dynamic_cast<ProtoField*>(sub);
  // generic is super of all fields
  if(!hoodtype) return true; if(!fsub->hoodtype) return false;
  return hoodtype->supertype_of(fsub->hoodtype);
}


// LEAST COMMON SUPERTYPE
ProtoType* ProtoType::lcs(ProtoType* t1, ProtoType* t2) {
  // easy case: if they're ordered, return the super
  if(t1->supertype_of(t2)) return t1;
  if(t2->supertype_of(t1)) return t2;
  // harder case: start w. one and walk up to find second
  return t1->lcs(t2);
}

ProtoType* ProtoLocal::lcs(ProtoType* t) { 
  if(!t->isA("ProtoLocal")) return ProtoType::lcs(t);
  return new ProtoLocal(); // no possible substructure
}

// Tuple/vector LCS
// Generalize elts to minimum fixed length, then generalize rests
// e.g. T<3,4,5> + T<3,6> -> T<3,Scalar,Scalar...>

void element_lcs(ProtoTuple *a, ProtoTuple* b, ProtoTuple* out) {
  int nfixed = MIN(a->types.size()-(!a->bounded),b->types.size()-(!b->bounded));
  for(int i=0;i<nfixed;i++) 
    out->types.push_back(ProtoType::lcs(a->types[i],b->types[i]));
  // make a "rest" from remaining types, when needed
  ProtoType* rest = NULL;
  for(int j=nfixed;j<a->types.size();j++)
    { if(!rest) rest = a->types[j]; else rest=ProtoType::lcs(a->types[j],rest);}
  for(int j=nfixed;j<b->types.size();j++)
    { if(!rest) rest = b->types[j]; else rest=ProtoType::lcs(b->types[j],rest);}
  if(rest!=NULL) { out->bounded=false; out->types.push_back(rest); }
  else out->bounded=true;
}

ProtoType* ProtoTuple::lcs(ProtoType* t) {
  if(!t->isA("ProtoTuple")) return ProtoLocal::lcs(t);
  ProtoTuple* tt = dynamic_cast<ProtoTuple*>(t);
  ProtoTuple* nt = new ProtoTuple(true); element_lcs(this,tt,nt); return nt;
}

ProtoType* ProtoSymbol::lcs(ProtoType* t) {
  if(!t->isA("ProtoSymbol")) return ProtoLocal::lcs(t);
  return new ProtoSymbol(); // LCS -> not super -> different symbols
}

ProtoType* ProtoNumber::lcs(ProtoType* t) { 
  if(!t->isA("ProtoNumber")) return ProtoLocal::lcs(t);
  return new ProtoNumber(); // no possible substructure
}

ProtoType* ProtoScalar::lcs(ProtoType* t) {
  if(!t->isA("ProtoScalar")) return ProtoNumber::lcs(t);
  return new ProtoScalar(); // LCS -> not super -> different values
}

ProtoType* ProtoBoolean::lcs(ProtoType* t) {
  if(!t->isA("ProtoBoolean")) return ProtoScalar::lcs(t);
  return new ProtoBoolean(); // LCS -> not super -> one true, other false
}

ProtoType* ProtoVector::lcs(ProtoType* t) {
  if(!t->isA("ProtoVector")) { // split inheritance
    if(t->isA("ProtoNumber")) return ProtoNumber::lcs(t);
    if(t->isA("ProtoTuple")) return ProtoTuple::lcs(t);
    return ProtoLocal::lcs(t); // join point in inheritance
  }
  ProtoVector* tv = dynamic_cast<ProtoVector*>(t);
  ProtoVector* nv = new ProtoVector(true); element_lcs(this,tv,nv); return nv;
}

ProtoType* ProtoLambda::lcs(ProtoType* t) {
  if(!t->isA("ProtoLambda")) return ProtoLocal::lcs(t);
  return new ProtoLambda(); // LCS -> not super -> different ops
}

ProtoType* ProtoField::lcs(ProtoType* t) {
  if(!t->isA("ProtoField")) return ProtoType::lcs(t);
  ProtoField* tf = dynamic_cast<ProtoField*>(t); //not super -> hoodtype != null
  return new ProtoField(ProtoType::lcs(hoodtype,tf->hoodtype));
}


// GREATEST COMMON SUBTYPE
ProtoType* ProtoType::gcs(ProtoType* t1, ProtoType* t2) {
  // easy case: if they're ordered, return the super
  if(t1->supertype_of(t2)) return t2;
  if(t2->supertype_of(t1)) return t1;
  // harder case: start w. one and walk down to find second
  return t1->gcs(t2);
}

ProtoType* ProtoType::gcs(ProtoType* t) {
  ierror("GCS dispatch failed for "+this->to_str()+" and "+t->to_str());
}
ProtoType* ProtoLocal::gcs(ProtoType* t) { return NULL; }
ProtoType* ProtoSymbol::gcs(ProtoType* t) { return NULL; }
ProtoType* ProtoScalar::gcs(ProtoType* t) { return NULL; } // covers boolean
ProtoType* DerivedType::gcs(ProtoType* t) { return NULL; }

bool element_gcs(ProtoTuple *a, ProtoTuple* b, ProtoTuple* out) {
  out->bounded = a->bounded || b->bounded;
  int afix=a->types.size()-(!a->bounded), bfix=b->types.size()-(!b->bounded);
  int nfixed = MIN(afix, bfix);
  if((a->bounded && afix<bfix) || (b->bounded && bfix<afix)) return false;
  // start by handling the fixed portion
  for(int i=0;i<nfixed;i++) {
    ProtoType* sub = ProtoType::gcs(a->types[i],b->types[i]);
    if(sub) out->add(sub); else return false;
  }
  // next, handle any excess rest portion
  ProtoTuple* longer = (afix>bfix) ? a : b;
  ProtoType* ctrest = (afix>bfix) ? b->types[bfix] : a->types[afix];
  for(int i=nfixed;i<longer->types.size()-(!longer->bounded);i++) {
    ProtoType* sub = ProtoType::gcs(longer->types[i],ctrest);
    if(sub) out->add(sub); else return false;
  }
  // last, handle the rest type
  if(!out->bounded) {
    ProtoType* sub = ProtoType::gcs(a->types[afix],b->types[bfix]);
    if(sub) out->add(sub); else return false;
  }
  return true;
}

ProtoType* ProtoTuple::gcs(ProtoType* t) { // covers vectors too
  if(t->isA("ProtoTuple")) {
    ProtoTuple* tt = dynamic_cast<ProtoTuple*>(t);
    bool vec = (isA("ProtoVector") || t->isA("ProtoVector"));
    ProtoTuple* newt;
    if(vec) newt = new ProtoVector(false); else newt = new ProtoTuple(false);
    if(!element_gcs(this,tt,newt)) { delete newt; return NULL; }
    return newt; // vector contents assured by GCS
  }
  if(t->type_of()=="ProtoNumber") {
    // check if all elements are scalars... if so, result is a vector
    ProtoVector *newv = new ProtoVector(bounded);
    ProtoScalar *constraint = new ProtoScalar(); bool ctused=false;
    for(int i=0;i<types.size();i++) {
      ProtoType* sub = ProtoType::gcs(types[i],constraint);
      if(sub==constraint) ctused=true; // don't delete if needed
      if(sub) newv->add(sub); else {delete newv;delete constraint; return NULL;}
    }
    if(!ctused) delete constraint;
    return newv;
  }
  return NULL;
}
ProtoType* ProtoNumber::gcs(ProtoType* t) {
  if(t->isA("ProtoTuple")) return t->gcs(this); // write it once, thank you
  return NULL; // otherwise, since not sub/super relation, is conflict
}
ProtoType* ProtoLambda::gcs(ProtoType* t) {
  if(!t->isA("ProtoLambda")) return NULL;
  return NULL; // since not sub/super relation, is conflict
}
ProtoType* ProtoField::gcs(ProtoType* t) {
  if(!t->isA("ProtoField")) return NULL;
  ProtoField* tf = dynamic_cast<ProtoField*>(t); //not super -> hoodtype != null
  ProtoType* hood = ProtoType::gcs(tf->hoodtype,hoodtype);
  return (hood==NULL) ? NULL : new ProtoField(hood);
}


// TYPE INTERPRETATION
bool DerivedType::is_arg_ref(string s) {
  if(s.size()<4) return false;
  if(s=="args") return true;
  if(s=="value") return true;
  if(s.substr(0,3)=="arg") {
    string num = s.substr(3,s.size()-3);
    if(str_is_number(num.c_str())) {
      int n = atoi(num.c_str());
      return (n>0 || num=="0");
    }
  }
  return false;
}

ProtoType* ProtoInterpreter::sexp_to_type(SExpr* s) {
  if(s->isSymbol()) {
    string name = ((SE_Symbol*)s)->name;
    if(name=="any") { return new ProtoType();
    } else if(name=="local") { return new ProtoLocal();
    } else if(name=="tuple") { return new ProtoTuple();
    } else if(name=="symbol") { return new ProtoSymbol();
    } else if(name=="number") { return new ProtoNumber();
    } else if(name=="scalar") { return new ProtoScalar();
    } else if(name=="boolean") { return new ProtoBoolean();
    } else if(name=="vector") { return new ProtoVector();
    } else if(name=="lambda" || name=="fun") { return new ProtoLambda();
    } else if(name=="field") { return new ProtoField();
    } else if(name=="return" || DerivedType::is_arg_ref(name)) {
      return new DerivedType(s);
    } else { return type_err(s,"Unknown type "+s->to_str());
    }
  } else if(s->isList()) {
    SE_List* sl = (SE_List*)s;
    if(!sl->op()->isSymbol()) 
      return type_err(s,"Compound type must start with symbol: "+s->to_str());
    string name = ((SE_Symbol*)sl->op())->name;
    if(name=="tuple" || name=="vector") {
      ProtoTuple* t;
      if(name=="tuple") t=new ProtoTuple(true); else t=new ProtoVector(true);
      for(int i=1;i<sl->len();i++) {
        SExpr* subex = (*sl)[i];
        if(subex->isSymbol() && ((SE_Symbol*)subex)->name=="&rest") {
          t->bounded=false; continue;
        }
        ProtoType* sub = sexp_to_type(subex);
        if(name=="vector" && !sub->isA("ProtoScalar"))
          return type_err(sl,"Vectors must contain only scalars");
        t->types.push_back(sub);
      }
      return t;
    } else if(name=="lambda" || name=="fun") {
      if(sl->len()!=3) return type_err(s,"Bad lambda type: "+s->to_str());
      Signature* sig = sexp_to_sig((*sl)[1]);
      sig->output = sexp_to_type((*sl)[2]);
      return new ProtoLambda(new Operator(sig));
    } else if(name=="field") {
      if(sl->len()!=2) return type_err(s,"Bad field type: "+s->to_str());
      ProtoType* sub = sexp_to_type((*sl)[1]);
      if(sub->isA("ProtoField")) 
        return type_err(s,"Field type must have a local subtype");
      return new ProtoField(sub);
    } else { // assume it's a derived type
      return new DerivedType(s);
    }
  } else {
    return new ProtoScalar(((SE_Scalar*)s)->value);
  }
}

/*****************************************************************************
 *  MACROS                                                                   *
 *****************************************************************************/

// gensyms give guaranteed unique names, used to prevent variable capture
int MacroOperator::gensym_count=0;
bool is_gensym(SE_Symbol* s) { return s->name[0]=='?'; }
bool is_gensym(SExpr* s) { return s->isSymbol() && is_gensym((SE_Symbol*)s); }
SE_Symbol* make_gensym(string root) {
  return new SE_Symbol(root+"~"+int2str(MacroOperator::gensym_count++));
}

// Macro signatures are simpler because they are syntactic, not semantic
Signature* ProtoInterpreter::sexp_to_macro_sig(SExpr* s) {
  if(!s->isList()) return sig_err(s,"Signature not a list: "+s->to_str());
  SE_List* sl = (SE_List*)s;  Signature* sig = new Signature();
  int stage = 0; // 0=required, 1=optional, 2=rest
  vector<SExpr*>::iterator it;
  for(it=sl->children.begin(); it<sl->children.end(); it++) {
    if(!(*it)->isSymbol()) 
      return sig_err(s,"Bad signature structure: "+s->to_str());
    string name = ((SE_Symbol*)*it)->name;
    if(name=="&optional") {
      if(stage>0) return sig_err(s,"Bad signature structure: "+s->to_str());
      stage=1; continue;
    } else if(name=="&rest") {
      if(stage>1) return sig_err(s,"Bad signature structure: "+s->to_str());
      stage=2; continue;
    } 
    // non-special symbols fall through to here
    ProtoType* type = new ProtoSymbol(name);
    switch(stage) {
    case 0: sig->required_inputs.push_back(type); break;
    case 1: sig->optional_inputs.push_back(type); break;
    case 2: sig->rest_input = type; stage=3; break; // only one rest
    case 3: return sig_err(s,"Bad signature structure: "+s->to_str());
    default: ierror("Unknown stage parsing macro signature: "+s->to_str());
    }
  }
  return sig;
}

Macro* ProtoInterpreter::sexp_to_macro(SE_List* s, Env *env) {
  if(!(*s)[1]->isSymbol()) 
    return macro_err(s,"Bad macro name: "+(*s)[1]->to_str());
  string name = ((SE_Symbol*)(*s)[1])->name;
  Macro* m;
  if(s->len()==3) { // symbol-style macro
    if(is_special((*s)[2])) 
      return macro_err(s,"Cannot rename specials: "+(*s)[1]->to_str());
    m = new MacroSymbol(name,(*s)[2]);
  } else if(s->len()==4) { // operator-style macro
    Signature *sig = sexp_to_macro_sig((*s)[2]);
    m = new MacroOperator(name,sig,(*s)[3]);
    env->force_bind(name,m); return m; // bind & return
  } else {
    return macro_err(s,"Macro must have 3 or 4 arguments: "+s->to_str());
  }
  env->force_bind(name,m); return m; // bind & return
}

SExpr* ProtoInterpreter::expand_macro(MacroOperator* m, SE_List* call) {
  Env m_env(this);
  // bind variables to SExprs
  if(!m->signature->legal_length(call->len()-1))
    return sexp_err(call,"Wrong number of arguments for macro "+m->name);
  int i=1; // start after macro name
  for(int j=0;j<m->signature->required_inputs.size();j++) {
    ProtoSymbol* var = dynamic_cast<ProtoSymbol*>(m->signature->required_inputs[j]);
    m_env.bind(var->value,(*call)[i++]);
  }
  for(int j=0;j<m->signature->optional_inputs.size() && i<call->len();j++) {
    ProtoSymbol* var = dynamic_cast<ProtoSymbol*>(m->signature->optional_inputs[j]);
    m_env.bind(var->value,(*call)[i++]);
  }
  if(i<call->len()) { // sweep all else into rest argument
    ProtoSymbol* var = dynamic_cast<ProtoSymbol*>(m->signature->rest_input);
    SE_List *rest = new SE_List(); rest->inherit_attributes(m);
    for(; i<call->len(); ) { rest->add((*call)[i++]); }
    m_env.bind(var->value,rest);
  }
  // then substitute the pattern
  return macro_substitute(m->pattern,&m_env);
}

// walks through, copying (and inheriting attributes)
SExpr* ProtoInterpreter::macro_substitute(SExpr* src, Env* e, SE_List* wrapper) {
  if(is_gensym(src)) { // substitute with a gensym for this instance
    SE_Symbol *groot = (SE_Symbol*)src;
    SExpr* gensym = (SExpr*)e->lookup(groot->name);
    if(!gensym) { // if gensym not yet created, make & bind it
      gensym = make_gensym(groot->name); gensym->inherit_attributes(src);
      e->bind(groot->name,gensym);
    }
    return gensym;
  } else if(src->isList()) { // SE_List
    SE_List *srcl = (SE_List*)src;
    string opname = (*srcl)[0]->isSymbol() ? ((SE_Symbol*)(*srcl)[0])->name :"";
    if(opname=="comma") {
      if(srcl->len()!=2 || !(*srcl)[1]->isSymbol())
        return sexp_err(src,"Bad comma form: "+src->to_str());
      SE_Symbol* sn = (SE_Symbol*)(*srcl)[1];
      return ((SExpr*)e->lookup(sn,"SExpr"))->copy(); // insert source text
    } else if(opname=="comma-splice") {
      if(wrapper==NULL)
        return sexp_err(src,"Comma-splice "+(*srcl)[0]->to_str()+" w/o list");
      if(srcl->len()!=2 || !(*srcl)[1]->isSymbol())
        return sexp_err(src,"Bad comma form: "+src->to_str());
      SE_Symbol* sn = (SE_Symbol*)(*srcl)[1];
      SE_List* value = (SE_List*)e->lookup(sn,"SE_List");
      for(int i=0;i<value->len();i++) 
        wrapper->add((*value)[i]->copy());
      return NULL; // comma-splices return null
    } else { // otherwise, just substitute each child
      SE_List *l = new SE_List(), *srcl = (SE_List*)src;
      for(int i=0;i<srcl->len();i++) {
        SExpr* sub = macro_substitute((*srcl)[i],e,l);
        if(sub!=NULL) l->add(sub); // comma-splices add selves and return null
      }
      return l;
    }
  } else { // symbol or scalar
    return src->copy();
  }
}

/*****************************************************************************
 *  INTERPRETER CORE                                                         *
 *****************************************************************************/

// if bindloc = NULL, then it's a primitive and expressions are types
// otherwise, it's a normal signature, and they're variables to be bound
Signature* ProtoInterpreter::sexp_to_sig(SExpr* s, Env* bindloc, CompoundOp* op, AM* space){
  if(!s->isList()) return sig_err(s,"Signature not a list: "+s->to_str());
  SE_List* sl = (SE_List*)s;  Signature* sig = new Signature();
  int stage = 0; // 0=required, 1=optional, 2=rest
  int varid = 0; // index to current parameter
  vector<SExpr*>::iterator it;
  for(it=sl->children.begin(); it<sl->children.end(); it++) {
    if((*it)->isSymbol()) {
      string name = ((SE_Symbol*)*it)->name;
      if(name=="&optional") {
        if(stage>0) return sig_err(s,"Bad signature structure: "+s->to_str());
        stage=1; continue;
      } else if(name=="&rest") {
        if(stage>1) return sig_err(s,"Bad signature structure: "+s->to_str());
        stage=2; continue;
      }
    } // non-special symbols fall through to here
    ProtoType* type = bindloc?(new ProtoType()):sexp_to_type(*it);
    if(bindloc) {
      if(!(*it)->isSymbol()) 
        return sig_err(*it,"Bad parameter: "+(*it)->to_str());
      Parameter *p = new Parameter(op,((SE_Symbol*)*it)->name,varid++);
      OperatorInstance *oi = new OperatorInstance(p,space);
      Field* f = op->body->inherit_and_add(*it,oi);
      bindloc->bind(p->name,f);
    }
    switch(stage) {
    case 0: sig->required_inputs.push_back(type); break;
    case 1: sig->optional_inputs.push_back(type); break;
    case 2: sig->rest_input = type; stage=3; break; // only one rest
    case 3: return sig_err(s,"Bad signature structure: "+s->to_str());
    default: ierror("Unknown stage while parsing signature: "+s->to_str());
    }
  }
  return sig;
}

Operator* ProtoInterpreter::sexp_to_op(SExpr* s, Env *env) {
  if(s->isSymbol()) {
    return (Operator*)env->lookup((SE_Symbol*)s,"Operator");
  } else if(s->isScalar()) {return op_err(s,s->to_str()+" is not an Operator");
  } else { // it must be a list
    SE_List *sl = (SE_List*)s;
    if(!sl->op()->isSymbol()) ierror("Tried to interpret "+s->to_str()+"as op");
    string opdef = ((SE_Symbol*)sl->op())->name;
    // (def name sig body) - constructs operator & graph, then binds
    // (lambda sig body) - constructs operator & graph
    if(opdef=="def" || opdef=="lambda" || opdef=="fun") {
      if(opdef=="def" && (sl->len()<4 || !sl->children[1]->isSymbol()))
        return op_err(s,"Malformed def: "+s->to_str());
      if((opdef=="lambda" || opdef=="fun") && sl->len()<3) 
        return op_err(s,"Malformed lambda: "+s->to_str());
      int i=1;
      string name; char tmp[40];
      if(opdef=="def") name=((SE_Symbol*)sl->children[i++])->name;
      else { sprintf(tmp,"lambda-%d",CompoundOp::lambda_count++); name=tmp; }
      // setup the operator & its signature
      CompoundOp* op = new CompoundOp(name); op->inherit_attributes(s);
      AM* funspace = new AM(op->body); op->body->spaces.insert(funspace);
      funspace->inherit_attributes(s);
      if(opdef=="def") env->force_bind(name,op);// bind early to allow recursion
      Env* inner = new Env(env);
      op->signature = sexp_to_sig(sl->children[i++],inner,op,funspace);
      op->signature->output = new DerivedType(new SE_Symbol("return"));
      // make body sexpr
      SE_List bodylist; bodylist.add(new SE_Symbol("all"));
      while(i<sl->len()) bodylist.add(sl->children[i++]);
      SExpr* body = (bodylist.len()==2)?bodylist.children[1]:&bodylist;
      // parsing body sexpr & collect real output value
      op->body->output = sexp_to_graph(body,op->body,funspace,inner);
      op->compute_side_effects();
      return op;
    } else if(opdef=="primitive") { // constructs & binds operator w/o DFG
      // (primitive name sig out &optional :side-effect)
      if(sl->len()<4 || !sl->children[1]->isSymbol())
        return op_err(s,"Malformed primitive: "+s->to_str());
      string pname = ((SE_Symbol*)sl->children[1])->name;
      Signature* sig = sexp_to_sig(sl->children[2]);
      sig->output = sexp_to_type(sl->children[3]);
      Operator* p  = new Primitive(pname,sig); p->inherit_attributes(s);
      // add in attributes
      for(int i=4;i<sl->len();i++) {
        SExpr* v = sl->children[i];
        if(!v->isKeyword()) return op_err(v,v->to_str()+" not a keyword");
        if(sl->len()>(i+1) && !sl->children[i+1]->isKeyword()) {
          p->attributes[((SE_Symbol*)v)->name]
            = new SExprAttribute(sl->children[i+1]);
          i++;
        } else {
          p->attributes[((SE_Symbol*)v)->name]=new MarkerAttribute(true);
        }
      }
      env->force_bind(pname,p); return p;
    } else {
      // check if it's a macro
      CompilationElement* ce = env->lookup(opdef);
      if(ce && ce->isA("Macro")) {
        SExpr* new_expr;
        if(ce->isA("MacroOperator")) {
          new_expr = expand_macro((MacroOperator*)ce,sl);
          if(new_expr->attributes.count("DUMMY")) // Mark of a failure
            return op_err(s,"Macro expansion failed on "+s->to_str());
        } else { // it's a MacroSymbol
          new_expr = sl->copy();
          ((SE_List*)new_expr)->children[0]=((Macro*)ce)->pattern;
        }
        return sexp_to_op(new_expr,env);
      }
      return op_err(sl->op(),"Can't make an operator with "+sl->op()->to_str());
    }
  }
  ierror("Fell through sexp_to_op w/o returning for: "+s->to_str());
}

Field* ProtoInterpreter::let_to_graph(SE_List* s, DFG* g, AM* space, Env *env, 
                                 bool incremental) { // incremental -> let*
  if(s->len()<3 || !s->children[1]->isList())
    return field_err(s,"Malformed let statement: "+s->to_str());
  Env* child = new Env(env);
  vector<SExpr*>::iterator let_exps = s->args();
  // collect let declarations
  SE_List* decls = (SE_List*)*let_exps++;
  for(int i=0;i<decls->len();i++) {
    if((*decls)[i]->isList()) {
      SE_List* d = (SE_List*)(*decls)[i];
      if(d->len()==2 && (*d)[0]->isSymbol()) {
        Field* f = sexp_to_graph((*d)[1],g,space,(incremental?child:env));
        child->bind(((SE_Symbol*)(*d)[0])->name,f);
      } else compile_error(d,"Malformed let statement: "+d->to_str());
    } else compile_error((*decls)[i],"Malformed let statement: "+
                         (*decls)[i]->to_str());
  }
  // evaluate body in child environment, returning last output
  Field* out=NULL;
  for(; let_exps<s->children.end(); let_exps++)
    out = sexp_to_graph(*let_exps,g,space,child);
  return out;
}

// A tuple-style letfed has the following behavior:
// 1. its MUX outputs a tuple
// 2. it decomposes said tuple to bind children
// returns true if binding's OK
bool bind_letfed_vars(SExpr* s, Field* val, DFG* g, AM* space, Env* env) {
  bool ok=false;
  if(s->isSymbol()) { // base case: just bind the variable
    env->bind(((SE_Symbol*)s)->name,val); ok=true;
  } else if(s->isList()) { // tuple variable?
    SE_List *sl = (SE_List*)s;
    if(sl->op()->isSymbol() && ((SE_Symbol*)sl->op())->name=="tup") {
      ok=true;
      for(int i=1;i<sl->len();i++) {
        // make element accessor
        OperatorInstance* accessor = new OperatorInstance(Env::ELT,space);
        Operator *lit = new Literal(new ProtoScalar(i-1));
        Field* index = g->inherit_and_add(s,new OperatorInstance(lit,space));
        accessor->add_input(val); accessor->add_input(index);
        Field* subval = g->inherit_and_add(s,accessor);
        // recurse binding process
        ok &= bind_letfed_vars((*sl)[i],subval,g,space,env);
      }
    }
  } // unhandled = not OK
  return ok; 
}

Field* ProtoInterpreter::letfed_to_graph(SE_List* s, DFG* g, AM* space, Env *env) {
  if(s->len()<3 || !s->children[1]->isList())
    return field_err(s,"Malformed letfed statement: "+s->to_str());
  Env *child = new Env(env), *delayed = new Env(env);
  // make the two forks: init and update
  OperatorInstance *dchange = new OperatorInstance(Env::DCHANGE,space);
  AM* init = new AM(space,g->inherit_and_add(s,dchange));
  OperatorInstance *unchange = new OperatorInstance(Env::NOT,space);
  unchange->add_input(dchange->output);
  AM* update = new AM(space,g->inherit_and_add(s,unchange));
  init->inherit_attributes(s); update->inherit_attributes(s);
  g->spaces.insert(init); g->spaces.insert(update);
  
  // collect & bind let declarations, verify syntax
  vector<SExpr*>::iterator let_exps = s->args();
  SE_List* decls = (SE_List*)*let_exps++;
  vector<OperatorInstance*> vars;
  for(int i=0;i<decls->len();i++) {
    if((*decls)[i]->isList() && ((SE_List*)(*decls)[i])->len()==3) {
      SE_List* d = (SE_List*)(*decls)[i];
      // create the variable & bind it
      OperatorInstance *varmux = new OperatorInstance(Env::MUX,space);
      varmux->attributes["LETFED-MUX"] = new MarkerAttribute(false);
      varmux->add_input(dchange->output);
      Field* curval = g->inherit_and_add(d,varmux);
      vars.push_back(varmux);
      // create the delayed version
      OperatorInstance *delay = new OperatorInstance(Env::DELAY,update);
      delay->add_input(varmux->output);
      Field* oldval = g->inherit_and_add(d,delay);
      // bind the variables
      if(!(bind_letfed_vars(d->op(),curval,g,space,child) &&
           bind_letfed_vars(d->op(),oldval,g,update,delayed)))
        compile_error(d,"Malformed letfed variable: "+d->op()->to_str());
      // evaluate & connect the init
      varmux->add_input(sexp_to_graph((*d)[1],g,init,env));
    } else compile_error((*decls)[i],"Malformed letfed statement: "+
                         (*decls)[i]->to_str());
  }
  // second pass to evalute update expressions
  for(int i=0;i<decls->len();i++) {
    SExpr* up = (*((SE_List*)(*decls)[i]))[2];
    vars[i]->add_input(sexp_to_graph(up,g,update,delayed));
  }
  // evaluate body in child environment, returning last output
  Field* out=NULL;
  for(; let_exps<s->children.end(); let_exps++)
    out = sexp_to_graph(*let_exps,g,space,child);
  return out;
}

Field* ProtoInterpreter::restrict_to_graph(SE_List* s, DFG* g, AM* space, Env *env){
  if(s->len()!=3) 
    return field_err(s,"Malformed restrict statement: "+s->to_str());
  Field* selector = sexp_to_graph((*s)[1],g,space,env);
  AM* subspace = new AM(space,selector); g->spaces.insert(subspace);
  subspace->inherit_attributes(s);
  return sexp_to_graph((*s)[2],g,subspace,env);
}

// Returns an instance of the literal if it exists, or else NULL
Operator* ProtoInterpreter::symbolic_literal(string name) {
  if(name=="true") { return new Literal(new ProtoBoolean(true));
  } else if(name=="false") { return new Literal(new ProtoBoolean(false));
  } else return NULL;
}

ProtoLocal* quote_to_literal_type(SExpr* s) {
  if(s->isSymbol()) {
    return new ProtoSymbol(((SE_Symbol*)s)->name);
  } else if(s->isScalar()) {
    return new ProtoScalar(((SE_Scalar*)s)->value);
  } else { // SE_List
    SE_List* sl = (SE_List*)s;
    vector<ProtoType*> subs; bool all_scalar=true;
    for(int i=0;i<sl->len();i++) {
      ProtoType* sub = quote_to_literal_type((*sl)[i]); subs.push_back(sub);
      if(!sub->isA("ProtoScalar")) all_scalar=false;
    }
    ProtoTuple* out = all_scalar ? new ProtoVector(true) : new ProtoTuple(true);
    for(int i=0;i<subs.size();i++) out->add(subs[i]);
    return out;
  }
}

// returns the output field
Field* ProtoInterpreter::sexp_to_graph(SExpr* s, DFG* g, AM* space, Env *env) {
  if(s->isSymbol()) {
    // All other symbols are looked up in the environment
    CompilationElement* elt = env->lookup(((SE_Symbol*)s)->name);
    if(elt==NULL) { 
      Operator* lit = symbolic_literal(((SE_Symbol*)s)->name);
      if(lit) return g->inherit_and_add(s,new OperatorInstance(lit,space));
      return field_err(s,"Couldn't find definition of "+s->to_str());
    } else if(elt->isA("Field")) { 
      Field* f = (Field*)elt;
      if(f->domain==space) { return f;
      } else if(space->child_of(f->domain)) { // implicit restriction
        OperatorInstance *oi = new OperatorInstance(Env::RESTRICT,space);
        oi->add_input(space->selector); oi->add_input(f);
        return g->inherit_and_add(s,oi);
      } else {
        ierror("Field "+f->to_str()+"'s domain not parent of "+space->to_str());
      }
    } else if(elt->isA("Operator")) {
      Operator* lambda = new Literal(new ProtoLambda((Operator*)elt));
      OperatorInstance* oi = new OperatorInstance(lambda,space);
      if(elt->isA("CompoundOp")) g->add_funcalls((CompoundOp*)elt,oi);
      return g->inherit_and_add(s,oi);
    } else if(elt->isA("MacroSymbol")) {
      return sexp_to_graph(((MacroSymbol*)elt)->pattern,g,space,env);
    } else return field_err(s,"Can't interpret "+elt->type_of()+" "+
                            s->to_str()+" as field");
  } else if(s->isScalar()) {
    // Numbers are literals
    Operator *lit = new Literal(new ProtoScalar(((SE_Scalar*)s)->value));
    return g->inherit_and_add(s,new OperatorInstance(lit,space));
  } else { // it must be a list
    // Lists are special forms or function applicatios
    SE_List* sl = (SE_List*)s;
    if(sl->len()==0) return field_err(sl,"Expression has no members"); 
    if(sl->op()->isSymbol()) { 
      // check if it's a special form
      string opname = ((SE_Symbol*)sl->op())->name;
      if(opname=="let") { return let_to_graph(sl,g,space,env,false);
      } else if(opname=="let*") { return let_to_graph(sl,g,space,env,true);
      } else if(opname=="all") { // evaluate children, returning last field
        Field* last=NULL;
        for(int j=1;j<sl->len();j++) last = sexp_to_graph((*sl)[j],g,space,env);
        return last;
      } else if(opname=="restrict"){ return restrict_to_graph(sl,g,space,env);
      } else if(opname=="def" && sl->len()==3) { // variable definition
        SExpr *def=(*sl)[1], *exp=(*sl)[2];
        if(!def->isSymbol())
          return field_err(sl,"def name not a symbol: "+def->to_str());
        Field* f = sexp_to_graph(exp,g,space,env);
        env->force_bind(((SE_Symbol*)def)->name,f);
        return f;
      } else if(opname=="def" || opname=="primitive" || opname=="lambda" || opname=="fun") {
        Operator* op = sexp_to_op(s,env);
        if(!(opname=="lambda" || opname=="fun")) return NULL;
        Operator* lambda = new Literal(new ProtoLambda(op));
        return g->inherit_and_add(s,new OperatorInstance(lambda,space));
      } else if(opname=="letfed") {
        return letfed_to_graph(sl,g,space,env);
      } else if(opname=="macro") {
        sexp_to_macro(sl,env);
        return NULL;
      } else if(opname=="include") {
        for(int j=1;j<sl->len();j++) {
          SExpr *ex = (*sl)[j];
          if(ex->isSymbol()) interpret_file(((SE_Symbol*)ex)->name);
          else compile_error(ex,"File name "+ex->to_str()+" is not a symbol");
        }
        return NULL;
      } else if(opname=="quote") {
        if(sl->len()!=2) 
          return field_err(sl,"Quote requires one arguement: "+s->to_str());
        Operator *lit = new Literal(quote_to_literal_type((*sl)[1]));
        return g->inherit_and_add(s,new OperatorInstance(lit,space));
      } else if(opname=="quasiquote") {
        return field_err(sl,"Quasiquote only allowed in macros: "+sl->to_str());
      }
      // check if it's a macro
      CompilationElement* ce = env->lookup(opname);
      if(ce && ce->isA("Macro")) {
        SExpr* new_expr;
        if(ce->isA("MacroOperator")) {
          new_expr = expand_macro((MacroOperator*)ce,sl);
          if(new_expr->attributes.count("DUMMY")) // Mark of a failure
            return field_err(s,"Macro expansion failed on "+s->to_str());
        } else { // it's a MacroSymbol
          new_expr = sl->copy();
          ((SE_List*)new_expr)->children[0]=((Macro*)ce)->pattern;
        }
        return sexp_to_graph(new_expr,g,space,env);
      }
    }
    // if we didn't return yet, it's an ordinary composite expression
    Operator *op = sexp_to_op(sl->op(),env);
    OperatorInstance *oi = new OperatorInstance(op,space);
    if(op->isA("CompoundOp")) g->add_funcalls((CompoundOp*)op,oi);
    for(vector<SExpr*>::iterator it=sl->args(); it!=sl->children.end(); it++) {
      Field* sub = sexp_to_graph(*it,g,space,env);
      // operator defs, primitives, and macros return null & are ignored
      if(sub) oi->add_input(sub);
    }
    return g->inherit_and_add(s,oi);
  }
  ierror("Fell through sexp_to_graph w/o returning for: "+s->to_str());
}

// TOPLEVEL INTERPRETER CALLS
// Interpret acts by updating the contents of the main DFG
void ProtoInterpreter::interpret(SExpr* sexpr, bool recursed) {
  // interpret the expression
  main->output = sexp_to_graph(sexpr,main,allspace,toplevel);
  terminate_on_error();
  // if finishing, check output validity, and might dump output
  if(!recursed && is_dump_interpretation) main->print_with_funcalls(cpout);
}

void ProtoInterpreter::interpret_file(string name) {
  ifstream* filestream = parent->proto_path.find_in_path(name);
  if(filestream==NULL)
    { compile_error("Can't find file '"+name+"'"); terminate_on_error(); }
  SExpr* sexpr= read_sexpr(name,filestream);
  compiler_error|=!sexpr; terminate_on_error();
  interpret(sexpr,true);
}


/*****************************************************************************
 *  EXTERNAL INTERFACE                                                       *
 *****************************************************************************/

ProtoInterpreter::ProtoInterpreter(NeoCompiler* parent, Args* args) {
  this->parent=parent;
  is_dump_interpretation = args->extract_switch("-CDinterpreted") 
    | parent->is_dump_all;

  // initialize compiler variables
  toplevel = new Env(this); main = new DFG();
  allspace = new AM(main); main->spaces.insert(allspace);
  allspace->attributes["CONTEXT"]=new Context("root-space",0);
  
  // load operators needed by compiler
  interpret_file("bootstrap.proto");
  Env::RESTRICT = (Operator*)toplevel->lookup("restrict");
  Env::DELAY = (Operator*)toplevel->lookup("delay");
  Env::DCHANGE = (Operator*)toplevel->lookup("dchange");
  Env::NOT = (Operator*)toplevel->lookup("not");
  Env::MUX = (Operator*)toplevel->lookup("mux");
  Env::TUP = (Operator*)toplevel->lookup("tup");
  Env::ELT = (Operator*)toplevel->lookup("elt");
  Env::LOCAL = (Operator*)toplevel->lookup("local");
  if(!(Env::RESTRICT && Env::DELAY && Env::DCHANGE && Env::NOT &&
       Env::MUX && Env::TUP && Env::ELT && Env::LOCAL))
    ierror("Compiler failed to parse bootstrap operators.");
  
  // load rest of operators
  populate_specials(); // make sure the key operators can't be shadowed
  interpret_file("core.proto");
}

ProtoInterpreter::~ProtoInterpreter() {
  delete toplevel; delete allspace;
}

void ProtoInterpreter::interpret(SExpr* sexpr) { interpret(sexpr,false); }

/*****************************************************************************
 *  PRINTING                                                                 *
 *****************************************************************************/
void ProtoTuple::print(ostream* out) { 
  *out << "<"; if(bounded) *out << types.size() << "-"; *out << "Tuple";
  for(int i=0;i<types.size();i++) { if(i) *out<<","; types[i]->print(out); }
  if(!bounded) *out<<"...";
  *out << ">";
}
void ProtoBoolean::print(ostream* out) {
  *out << "<Boolean"; 
  if(constant) { *out << " " << (value?"true":"false"); } 
  *out << ">";
}
void ProtoVector::print(ostream* out) {
  *out << "<"; if(bounded) *out << types.size() << "-"; *out << "Vector";
  for(int i=0;i<types.size();i++) { if(i) *out<<","; types[i]->print(out); }
  if(!bounded) *out<<"...";
  *out << ">";
}
void ProtoLambda::print(ostream* out)
{ *out << "<Lambda"; if(op) { *out << " "; op->print(out); } *out << ">"; }

void Parameter::print(ostream* out) { 
  *out << "[Parameter " << index << ": "<< name;
  if(defaultValue) defaultValue->print(out);
  *out<<"]";
}

void Field::print(ostream* out)
{ *out<<nicename(this)<<": "<<nicename(domain)<<" --> "; range->print(out); }

void AM::print(ostream* out) {
  *out << "[Medium: " << nicename(this) << " = ";
  if(parent) { *out << nicename(parent) << " | ";
    if(selector) { *out << nicename(selector); }
  } else { *out << "root"; }
  *out << "]";
}
void Signature::print(ostream* out) {
  *out << "[Signature: ";
  bool first=true;
  for(int i=0;i<required_inputs.size();i++)
    { if(first) first=false; else *out<<" "; required_inputs[i]->print(out); }
  if(optional_inputs.size()) 
    { if(first) first=false; else *out<<" "; *out<<"&optional"; }
  for(int i=0;i<optional_inputs.size();i++) 
    { *out << " "; optional_inputs[i]->print(out);}
  if(rest_input) {
    if(first) first=false; else *out<<" "; *out<<"&rest ";
    rest_input->print(out);
  }
  *out << " --> ";
  output->print(out);
  *out << "]";
}

void OperatorInstance::print(ostream* out) {
  for(int i=0;i<inputs.size();i++) {
    if(i) *out << ", ";
    *out << nicename(inputs[i]); inputs[i]->range->print(out);
  }
  if(inputs.size()) *out << " --> "; 
  op->print(out); 
  *out << " --> " << nicename(output); output->range->print(out);
}

void DFG::print(ostream* out) {
  *out << pp_indent() << "Amorphous Mediums:\n"; pp_push(2);
  set<AM*>::iterator ait;
  for(ait=spaces.begin(); ait!=spaces.end(); ait++) 
    { *out << pp_indent(); (*ait)->print(out); *out << endl; }
  pp_pop(); *out << pp_indent()  << "Fields:\n"; pp_push(2);
  set<Field*>::iterator fit;
  for(fit=edges.begin(); fit!=edges.end(); fit++) 
    { *out << pp_indent(); (*fit)->print(out); 
      if((*fit)==output) *out<< " OUTPUT"; *out << endl; }
  pp_pop(); *out << pp_indent() << "Operator Instances:\n"; pp_push(2);
  set<OperatorInstance*, CompilationElement_cmp>::iterator oit;
  for(oit=nodes.begin(); oit!=nodes.end(); oit++) 
    { *out << pp_indent(); (*oit)->print(out); 
      if((*oit)->output==output) *out<< " OUTPUT"; *out << endl; }
  pp_pop();
}

void DFG::print_with_funcalls(ostream* out) {
  map<Operator*,set<OperatorInstance*, CompilationElement_cmp> >::iterator i=funcalls.begin();
  for( ; i!=funcalls.end(); i++) {
    CompoundOp* op = (CompoundOp*)((*i).first);
    *cpout<<"Function: "<<op->name<<" called "<<(*i).second.size()<<" times\n";
    pp_push(2); op->body->print(cpout); pp_pop();
  }
  print(cpout);
}


/*****************************************************************************
 *  HELPER MISCELLANY                                                        *
 *****************************************************************************/
// this is where we stick definition material for "ir.h"

bool Signature::legal_length(int n) { 
  if(n<required_inputs.size()) return false;
  if(rest_input==NULL && n>(required_inputs.size()+optional_inputs.size()))
    return false;
  return true;
}

AM::AM(AM* parent, Field* f) { 
  this->parent=parent; selector=f; f->selectors.push_back(this);
  parent->children.insert(this); container=parent->container;
}

int CompoundOp::lambda_count=0;

CompoundOp::CompoundOp(string n) { 
  name=n; body=new DFG(); body->container=this; signature=NULL; 
}
// looks through to see if any of its children has a side-effect
bool CompoundOp::compute_side_effects() {
  if(attributes[":side-effect"]) delete attributes[":side-effect"]; // clear old
  set<OperatorInstance*>::iterator oit;
  for(oit=body->nodes.begin(); oit!=body->nodes.end(); oit++) {
    if((*oit)->op->attributes.count(":side-effect")) {
      attributes[":side-effect"]=new MarkerAttribute(true); return true;
    }
  }
  attributes.erase(":side-effect"); return false;
}

// table of FieldOps used to date
map<Operator*,FieldOp*,CompilationElement_cmp> FieldOp::fieldops;
FieldOp* FieldOp::get_field_op(OperatorInstance* oi) {
  if(!oi->op->isA("Primitive") || oi->pointwise()==0) return NULL;
  // reuse or create appropriate FieldOp
  if(!fieldops.count(oi->op)) fieldops[oi->op] = new FieldOp(oi->op);
  return fieldops[oi->op];
}

// assumes base is pointwise
ProtoType* fieldop_type(ProtoType* base) {
  if(base->isA("DerivedType")) return base;
  else return new ProtoField(base);
}
FieldOp::FieldOp(Operator* base) {
  this->base = base; inherit_attributes(base);
  name = "Field~~"+base->name;
  // make field-ified version of signature
  Signature *b = base->signature;
  signature = new Signature(fieldop_type(b->output));
  for(int i=0;i<b->required_inputs.size();i++)
    signature->required_inputs.push_back(fieldop_type(b->required_inputs[i]));
  for(int i=0;i<b->optional_inputs.size();i++)
    signature->optional_inputs.push_back(fieldop_type(b->optional_inputs[i]));
  if(b->rest_input) signature->rest_input = fieldop_type(b->rest_input);
}


bool AM::child_of(AM* am) { 
  if(am->parent==NULL) return true;
  if(am==parent) return true;
  if(parent) return parent->child_of(am); else return false;
}


// pointwise test returns 1 if pointwise, 0 if not, and -1 if unresolved
int OperatorInstance::pointwise() {
  int opp = output->range->pointwise(); if(opp==0) return 0;
  if(op->isA("Literal") || op->isA("Parameter")) { 
    return opp; // Literal, Parameter: depends only on value
  } else if(op->isA("Primitive")) {
    if(op->attributes.count(":space") || op->attributes.count(":time") ||
       op->attributes.count(":side-effect"))
      return 0; // primitives involving space, time, actuators aren't pointwise
    return opp; // others may depend on what they're operating on...
  } else if(op->isA("CompoundOp")) {
    set<OperatorInstance*>::iterator i=((CompoundOp*)op)->body->nodes.begin();
    for(; i!=((CompoundOp*)op)->body->nodes.end();i++)
      {int inop = (*i)->pointwise(); if(inop==0) return 0; if(inop==-1) opp=-1;}
    return opp; // compound op is pointwise if all its contents are pointwise
  } else { // FieldOp, generic Operator
    return 0;
  }
}


DFG* DFG::instance() {
  map<CompilationElement*,CompilationElement*,CompilationElement_cmp> remap;
  DFG* child = new DFG();

  // duplicate all AMs
  for(set<AM*>::iterator ai = spaces.begin(); ai!=spaces.end(); ai++) {
    AM *newam = new AM(child);
    child->spaces.insert(newam); remap[*ai] = newam;
  }

  // duplicate all operators, recording their relations
  set<OperatorInstance*>::iterator ni = nodes.begin();
  for(; ni!=nodes.end(); ni++) {
    // duplicate the operator
    OperatorInstance *oi = (*ni);
    AM *newam = (AM*)remap[oi->output->domain];
    OperatorInstance *newoi = new OperatorInstance(oi->op,newam);
    child->inherit_and_add(oi,newoi);
    remap[oi] = newoi; remap[oi->output] = newoi->output;
    newoi->output->range = oi->output->range;
    // include funcalls
    if(newoi->op->isA("CompoundOp")) add_funcalls((CompoundOp*)newoi->op,newoi);
    // TODO: include literal lambda funcalls as well
  }

  // change the DFG output to a new field
  child->output = (Field*)remap[output]; 

  // now walk over all fields, connecting them up
  for(ni = nodes.begin(); ni!=nodes.end(); ni++) {
    OperatorInstance *oi = (*ni);
    OperatorInstance *newoi = (OperatorInstance*)remap[oi];
    for(int i=0;i<oi->inputs.size();i++) {
      Field* newf = (Field*)remap[oi->inputs[i]];
      newoi->inputs.push_back(newf); newf->use(newoi,i);
    }
  }

  // swap all AM pointers
  for(set<AM*>::iterator ai = spaces.begin(); ai!=spaces.end(); ai++) {
    AM *newam = (AM*)remap[*ai];
    if((*ai)->parent) {  // also implies selector is non-null
      // set parent (and backpointer)
      newam->parent = (AM*)remap[(*ai)->parent];
      newam->parent->children.insert(newam);
      // set selector (and backpointer)
      Field* newf = (Field*)remap[(*ai)->selector];
      newam->selector = newf; newf->selectors.push_back(newam);
    }
  }

  return child;
}

// Macro-ize a common sequence of operations
Field* DFG::inherit_and_add(CompilationElement* src, OperatorInstance* oi) {
  oi->inherit_attributes(src); oi->output->inherit_attributes(src);
  oi->container=this; oi->output->container=this;
  nodes.insert(oi); edges.insert(oi->output);
  return oi->output;
}

void DFG::add_funcalls(CompoundOp* lambda, OperatorInstance* oi) {
  funcalls[lambda].insert(oi);
  if(lambda->body!=this) { // don't import on recursive calls
    map<Operator*,set<OperatorInstance*, CompilationElement_cmp> >::iterator i;
    for(i=lambda->body->funcalls.begin();i!=lambda->body->funcalls.end();i++)
      funcalls[(*i).first].insert((*i).second.begin(),(*i).second.end());
  }
}

void DFG::relocate_input(OperatorInstance* src, int src_loc, OperatorInstance* dst,int dst_loc) {
  Field* f = src->inputs[src_loc];
  if(!f->consumers.erase(make_pair(src,src_loc)))
    ierror("Attempted to relocate output with missing trackbacks");
  insert_at(&dst->inputs,dst_loc,f); delete_at(&src->inputs,src_loc);
  for(int i=src_loc;i<src->inputs.size();i++) { // fix back-pointers
    src->inputs[i]->consumers.erase(make_pair(src,i+1));
    src->inputs[i]->consumers.insert(make_pair(src,i));
  }
  f->consumers.insert(make_pair(dst,dst_loc));
}
void DFG::relocate_inputs(OperatorInstance* src, OperatorInstance* dst,int insert) {
  while(!src->inputs.empty()) { relocate_input(src,0,dst,insert); insert++; }
}

void DFG::relocate_source(OperatorInstance* consumer, int in, Field* newsrc){
  Field* oldsrc = consumer->inputs[in];
  oldsrc->consumers.erase(make_pair(consumer,in));
  consumer->inputs[in] = newsrc;
  newsrc->consumers.insert(make_pair(consumer,in));
}

void DFG::relocate_consumers(Field* src, Field* dst) {
  set<pair<OperatorInstance*,int> >::iterator i;
  for(i=src->consumers.begin();i!=src->consumers.end();i++) {
    (*i).first->inputs[(*i).second]=dst;
    dst->consumers.insert(*i);
  }
  for(int i=0;i<src->selectors.size();i++) {
    src->selectors[i]->selector = dst; 
    dst->selectors.push_back(src->selectors[i]);
  }
  src->consumers.clear(); src->selectors.clear(); // purge old
  // now selectors
  if(src->container->output==src) src->container->output = dst; // move output
}

void recursive_funcall_delete(OperatorInstance* oi, DFG* g) {
  g->funcalls[oi->op].erase(oi);
  if(g->funcalls[oi->op].empty()) g->funcalls.erase(oi->op);
  if(g->container) {
    set<OperatorInstance*>::iterator i = g->container->usages.begin();
    for(; i!=g->container->usages.end(); i++) {
      recursive_funcall_delete(oi,(*i)->container);
    }
  }
}

void DFG::delete_node(OperatorInstance* oi) {
  // release the inputs
  for(int i=0;i<oi->inputs.size();i++)
    if(oi->inputs[i]) oi->inputs[i]->unuse(oi,i);
  // blank the consumers (which should be about to be deleted)
  set<pair<OperatorInstance*,int> >::iterator i;
  for(i=oi->output->consumers.begin();i!=oi->output->consumers.end();i++) {
    (*i).first->inputs[(*i).second] = NULL;
  }
  // remove the space's record of the field
  if(oi->output->domain) oi->output->domain->fields.erase(oi->output);
  // remove any compound-op references
  if(oi->op->isA("CompoundOp")) {
    ((CompoundOp*)oi->op)->usages.erase(oi);
    recursive_funcall_delete(oi,this);
  }
  // discard the elements
  nodes.erase(oi); edges.erase(oi->output);
  delete oi->output; delete oi; // finally, release our memory
}

void DFG::delete_space(AM* am) {
  // release the parent & selector
  if(am->parent) am->parent->children.erase(am);
  if(am->selector) {
    int sid = index_of(&am->selector->selectors,am);
    if(sid>=0) delete_at(&am->selector->selectors,sid);
  }
  // blank children and domains (which should be able to be deleted)
  for(set<AM*>::iterator i=am->children.begin();i!=am->children.end();i++)
    (*i)->parent=NULL;
  for(set<Field*>::iterator i=am->fields.begin();i!=am->fields.end();i++)
    (*i)->domain=NULL;
  // discard the element & release memory
  spaces.erase(am); delete am;
}

// can only inline 
bool DFG::make_op_inline(OperatorInstance* target) {
  if(!target->op->isA("CompoundOp")) return false;
  DFG* inlinefrag = ((CompoundOp*)target->op)->body->instance();
  
  // Remap fragment AM to be oi AM
  AM *oldam = inlinefrag->output->domain, *newam = target->output->domain;
  set<Field*>::iterator fi = oldam->fields.begin();
  for(; fi!=oldam->fields.end(); fi++)
    { (*fi)->domain = newam; newam->fields.insert(*fi); }
  set<AM*>::iterator ai = oldam->children.begin();
  for(; ai!=oldam->children.end(); ai++) 
    { (*ai)->parent = newam; newam->children.insert(*ai); }
  
  // Replace output
  relocate_consumers(target->output,inlinefrag->output);

  // Replace parameters; copy all other elements into this DFG
  set<OperatorInstance*>::iterator oi = inlinefrag->nodes.begin();
  for(; oi!=inlinefrag->nodes.end(); oi++) {
    if((*oi)->op->isA("Parameter")) {
      // WARNING: NOT HANDLING REST VARIABLES YET!
      (*oi)->output->domain->fields.erase((*oi)->output); // remove backpointer
      Field* newsrc = target->inputs[((Parameter*)(*oi)->op)->index];
      relocate_consumers((*oi)->output,newsrc);
    } else {
      nodes.insert(*oi); edges.insert((*oi)->output); 
      (*oi)->container = this; (*oi)->output->container = this;
      // import funcalls
      if((*oi)->op->isA("CompoundOp")) add_funcalls((CompoundOp*)(*oi)->op,*oi);
      // TODO: include literal lambda funcalls as well
    }
  }
  ai = inlinefrag->spaces.begin();
  for(; ai!=inlinefrag->spaces.end(); ai++)
    if((*ai)!=oldam) { spaces.insert(*ai); (*ai)->container = this; }
  
  // Discard old OI
  delete_node(target);
  // Done!
  return true;
}

// INTERNAL TESTING
string typeorder_test(ProtoType* ta,ProtoType* tb) {
  return ta->to_str()+" > "+tb->to_str()+": " + bool2str(ta->supertype_of(tb)) +
    ", reverse: " + bool2str(tb->supertype_of(ta));
}

string lcs_test(ProtoType* ta,ProtoType* tb) {
  ProtoType *tlcs = ProtoType::lcs(ta,tb), *tr = ProtoType::lcs(tb,ta);
  bool inv_ok = tlcs->supertype_of(tr) && tr->supertype_of(tlcs); // must be =
  return "LCS("+ta->to_str()+","+tb->to_str()+") = " + tlcs->to_str() +
    "; inverse match = "+bool2str(inv_ok);
}

string gcs_test(ProtoType* ta,ProtoType* tb) {
  ProtoType *tgcs = ProtoType::gcs(ta,tb), *tr = ProtoType::gcs(tb,ta);
  bool inv_ok = false; // is reverse same as forward?
  if(tgcs && tr) { inv_ok = tgcs->supertype_of(tr) && tr->supertype_of(tgcs);
  } else { inv_ok = tgcs==tr; }
  string gstr = (tgcs==NULL) ? "NULL" : tgcs->to_str();
  return "GCS("+ta->to_str()+","+tb->to_str()+") = " + gstr + 
    "; inverse match = "+bool2str(inv_ok);
}

void type_system_tests() {
  ProtoType top; ProtoLocal local; ProtoTuple tuple; ProtoSymbol symbol;
  ProtoNumber number; ProtoScalar scalar; ProtoBoolean boolean;
  ProtoVector vector; ProtoLambda lambda; ProtoField field;
  *cpout << "Testing type relations:\n";
  // begin with generic type comparison
  *cpout << "Generic types:\n";
  *cpout << typeorder_test(&top,&top) << endl;
  *cpout << typeorder_test(&top,&local) << endl;
  *cpout << typeorder_test(&top,&field) << endl;
  *cpout << typeorder_test(&top,&lambda) << endl;
  *cpout << typeorder_test(&top,&scalar) << endl;
  *cpout << typeorder_test(&scalar,&number) << endl;
  *cpout << typeorder_test(&scalar,&field) << endl;
  *cpout << typeorder_test(&vector,&tuple) << endl;
  *cpout << typeorder_test(&scalar,&boolean) << endl;
  *cpout << typeorder_test(&scalar,&symbol) << endl;
  *cpout << typeorder_test(&top,&vector) << endl;
  *cpout << typeorder_test(&vector,&number) << endl;
  *cpout << typeorder_test(&vector,&boolean) << endl;
  *cpout << typeorder_test(&symbol,&boolean) << endl;
  *cpout << typeorder_test(&symbol,&local) << endl;
  *cpout << typeorder_test(&field,&local) << endl;
  *cpout << typeorder_test(&lambda,&local) << endl;
  *cpout << typeorder_test(&tuple,&local) << endl;
  // next some literals
  *cpout << "Literals:\n";
  ProtoScalar l3(3), l4(4), l5(5), l1(1), l0(0);
  *cpout << typeorder_test(&l3,&local) << endl;
  *cpout << typeorder_test(&l4,&l5) << endl;
  *cpout << typeorder_test(&l4,&boolean) << endl;
  *cpout << typeorder_test(&scalar,&l3) << endl;
  ProtoBoolean bt(true), bf(false);
  *cpout << typeorder_test(&bt,&boolean) << endl;
  *cpout << typeorder_test(&bf,&symbol) << endl;
  *cpout << typeorder_test(&boolean,&bf) << endl;
  *cpout << typeorder_test(&bf,&bt) << endl;
  *cpout << typeorder_test(&bf,&l1) << endl; // bool/scalar comparisons
  *cpout << typeorder_test(&bf,&l0) << endl;
  *cpout << typeorder_test(&bt,&l1) << endl;
  *cpout << typeorder_test(&bt,&l0) << endl;
  ProtoSymbol sf("foo"), sb("bar");
  *cpout << typeorder_test(&sf,&number) << endl;
  *cpout << typeorder_test(&sb,&symbol) << endl;
  *cpout << typeorder_test(&top,&sf) << endl;
  *cpout << typeorder_test(&sf,&sb) << endl;
  // now compound types
  *cpout << "Tuples:\n";
  ProtoTuple t2(true); t2.add(&l3); t2.add(&symbol);
  ProtoTuple t2u(false); t2u.add(&l3); t2u.add(&top); t2u.add(&field);
  *cpout << typeorder_test(&t2,&t2u) << endl;
  *cpout << typeorder_test(&t2,&tuple) << endl;
  *cpout << typeorder_test(&t2u,&tuple) << endl;
  ProtoTuple t34s(false); t34s.add(&l3); t34s.add(&l4); t34s.add(&scalar);
  ProtoTuple t34ss(false); t34ss.add(&l3); t34ss.add(&l4); t34ss.add(&scalar); t34ss.add(&scalar);
  ProtoTuple t345s(false); t345s.add(&l3); t345s.add(&l4); t345s.add(&l5); t345s.add(&scalar);
  ProtoTuple t34(true); t34.add(&l3); t34.add(&l4);
  ProtoTuple t345(true); t345.add(&l3); t345.add(&l4); t345.add(&l5);
  ProtoTuple t3x5(true); t3x5.add(&l3); t3x5.add(&top); t3x5.add(&l5);
  ProtoTuple t3t5(true); t3t5.add(&l3); t3t5.add(&t345); t3t5.add(&l5);
  *cpout << typeorder_test(&tuple,&t34s) << endl;
  *cpout << typeorder_test(&t345s,&t34s) << endl;
  *cpout << typeorder_test(&t345,&t34s) << endl;
  *cpout << typeorder_test(&t3x5,&t34s) << endl;
  *cpout << typeorder_test(&t34,&t34s) << endl;
  *cpout << typeorder_test(&t3t5,&t34s) << endl;
  *cpout << typeorder_test(&t345,&t345s) << endl;
  *cpout << typeorder_test(&t3x5,&t345s) << endl;
  *cpout << typeorder_test(&t34,&t345s) << endl;
  *cpout << typeorder_test(&t3x5,&t345) << endl;
  *cpout << typeorder_test(&t3x5,&t34) << endl;
  *cpout << typeorder_test(&t3x5,&t3t5) << endl;
  *cpout << typeorder_test(&t34ss,&t34s) << endl;
  *cpout << typeorder_test(&t34ss,&t345s) << endl;
  *cpout << typeorder_test(&t34ss,&t345) << endl;
  *cpout << typeorder_test(&t34ss,&t34) << endl;
  *cpout << "Vectors:\n";
  ProtoVector v345(true); v345.add(&l3); v345.add(&l4); v345.add(&l5);
  ProtoVector v34s(false); v34s.add(&l3); v34s.add(&l4); v34s.add(&scalar);
  *cpout << typeorder_test(&v345,&t345) << endl;
  *cpout << typeorder_test(&v345,&t34) << endl;
  *cpout << typeorder_test(&v345,&t345s) << endl;
  *cpout << typeorder_test(&v345,&t3x5) << endl;
  *cpout << typeorder_test(&v34s,&v345) << endl;
  *cpout << typeorder_test(&v34s,&t345) << endl;
  *cpout << typeorder_test(&v34s,&t34s) << endl;
  *cpout << "Fields:\n";
  ProtoField f3(&l3), f3t5(&t3t5), f3x5(&t3x5);
  *cpout << typeorder_test(&f3,&field) << endl;
  *cpout << typeorder_test(&f3,&f3t5) << endl;
  *cpout << typeorder_test(&f3t5,&field) << endl;
  *cpout << typeorder_test(&f3t5,&f3x5) << endl;
  // LCS relations
  *cpout << "Testing least-common-supertype:\n";
  // first some ordered pairs
  *cpout << "LCS ordered pairs:\n";
  *cpout << lcs_test(&top,&f3) << endl; // = any
  *cpout << lcs_test(&l3,&local) << endl; // = local
  *cpout << lcs_test(&number,&boolean) << endl; // = number
  // now cross-class generalizations
  *cpout << "LCS cross-class:\n";
  *cpout << lcs_test(&f3t5,&t3t5) << endl; // = any
  *cpout << lcs_test(&bf,&t3t5) << endl; // = local
  *cpout << lcs_test(&boolean,&vector) << endl; // = number
  *cpout << lcs_test(&bt,&l5) << endl; // = scalar
  *cpout << lcs_test(&vector,&t3t5) << endl; // = tuple<Local...>
  *cpout << lcs_test(&boolean,&lambda) << endl; // = local
  *cpout << lcs_test(&vector,&field) << endl; // = any
  // finally, in-class generalizations
  *cpout << "LCS in-class generalization:\n";
  ProtoTuple t31(true); t31.add(&l3); t31.add(&l1);
  ProtoVector v145(true); v145.add(&l1); v145.add(&l4); v145.add(&l5);
  *cpout << lcs_test(&bf,&bt) << endl; // = boolean
  *cpout << lcs_test(&bt,&l1) << endl; // = <Scalar 1>
  *cpout << lcs_test(&l1,&l3) << endl; // = scalar
  *cpout << lcs_test(&t31,&t345) << endl; // = T<3,Sc,5...>
  *cpout << lcs_test(&t345s,&t3x5) << endl; // = T<3,Any,5,Sc...>
  *cpout << lcs_test(&t3t5,&t345) << endl; // = T<3,Local,5>
  *cpout << lcs_test(&t34ss,&t34) << endl; // = T<3,4,Sc...>
  *cpout << lcs_test(&v34s,&v145) << endl; // = V<Sc,4,Sc,Sc...>
  *cpout << lcs_test(&v145,&l3) << endl; // = number
  *cpout << lcs_test(&v145,&t345) << endl; // = T<Sc,4,5>
  *cpout << lcs_test(&sf,&sb) << endl; // = symbol
  *cpout << lcs_test(&f3,&f3t5) << endl; // = F<local>
  // GCS relations
  *cpout << "Testing greatest-common-subtype:" << endl;
  *cpout << "GCS ordered pairs:\n";
  *cpout << gcs_test(&top,&f3) << endl; // = <Field <Sc 3>>
  *cpout << gcs_test(&l3,&local) << endl; // = <Scalar 3>
  *cpout << gcs_test(&number,&boolean) << endl; // = boolean
  // now cross-class generalizations
  *cpout << "GCS cross-class:\n";
  *cpout << gcs_test(&f3t5,&t3t5) << endl; // = null
  *cpout << gcs_test(&vector,&t3t5) << endl; // = null
  *cpout << gcs_test(&boolean,&lambda) << endl; // = null
  *cpout << gcs_test(&vector,&field) << endl; // = null
  *cpout << gcs_test(&number,&tuple) << endl; // = V<Sc...>
  *cpout << gcs_test(&t3t5,&number) << endl; // = null
  *cpout << gcs_test(&t3x5,&number) << endl; // = V<3S5>
  // finally, in-class specializations
  *cpout << "GCS in-class specialization:" << endl;
  ProtoTuple t34x(false); t34x.add(&l3); t34x.add(&l4); t34x.add(&top);
  ProtoTuple t3x5s(false); t3x5s.add(&l3); t3x5s.add(&top); t3x5s.add(&l5); t3x5s.add(&scalar);
  ProtoTuple t3x5f(false); t3x5f.add(&l3); t3x5f.add(&top); t3x5f.add(&l5); t3x5f.add(&field);
  ProtoVector vs45(true); vs45.add(&scalar); vs45.add(&l4); vs45.add(&l5);
  *cpout << gcs_test(&bf,&bt) << endl; // = NULL
  *cpout << gcs_test(&bt,&l1) << endl; // = <Boolean true>
  *cpout << gcs_test(&l1,&l3) << endl; // = NULL
  *cpout << gcs_test(&t34,&t345) << endl; // = null
  *cpout << gcs_test(&t34,&t34s) << endl; // = T<3,4>
  *cpout << gcs_test(&t34,&t34ss) << endl; // = null
  *cpout << gcs_test(&t345s,&t3x5) << endl; // = T<3,4,5>
  *cpout << gcs_test(&t34x,&t34ss) << endl; // = T<3,4,Sc,Sc...>
  *cpout << gcs_test(&t3x5,&t34x) << endl; // = T<3,4,5>
  *cpout << gcs_test(&t3x5s,&t34x) << endl; // = T<3,4,5,Sc...>
  *cpout << gcs_test(&t3x5s,&t3x5f) << endl; // = null
  *cpout << gcs_test(&t3x5f,&t34x) << endl; // = T<3,4,5,Field...>
  *cpout << gcs_test(&v34s,&t3x5) << endl; // = V<3,4,5>
  *cpout << gcs_test(&t3x5,&v34s) << endl; // = V<3,4,5>
  *cpout << gcs_test(&v34s,&vs45) << endl; // = V<3,4,5>
  *cpout << gcs_test(&sf,&sb) << endl; // = null
  *cpout << gcs_test(&f3x5,&f3t5) << endl; // = F<T<3,T<3,4,5>,5>>
  *cpout << gcs_test(&tuple,&t34) << endl; // = T<3,4>
}
