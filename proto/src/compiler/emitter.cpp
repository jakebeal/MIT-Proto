/* ProtoKernel code emitter
Copyright (C) 2009, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// This turns a Proto program representation into ProtoKernel bytecode to
// execute it.

#include "config.h"
#include "neocompiler.h"
#include "proto_opcodes.h"

map<int,string> opnames;
map<string,int> primitive2op;
map<int,int> op_stackdeltas;

// instructions like SQRT_OP that have no special rules or pointers
class Instruction : public CompilationElement {
public:
  Instruction *next,*prev; // sequence links
  int location; // -1 = unknown
  set<Instruction*> dependents; // instructions "neighboring" this one
  
  OPCODE op;
  vector<uint8_t> parameters; // values consumed after op
  int stack_delta; // change in stack size following this instruction
  int env_delta; // change in environment size following this instruction
  Instruction(OPCODE op, int ed=0) {
    this->op=op; stack_delta=op_stackdeltas[op]; env_delta=ed; 
    location=-1; next=prev=NULL;
  }
  virtual bool isA(string c){return c=="Instruction"||CompilationElement::isA(c);}
  virtual void print(ostream* out=0) {
    *out << (opnames.count(op) ? opnames[op] : "<UNKNOWN OP>");
    for(int i=0;i<parameters.size();i++) { *out << ", " << i2s(parameters[i]); }
  }
  
  virtual int size() { return 1 + parameters.size(); }
  virtual bool resolved() { return location>=0; }
  virtual void output(uint8_t* buf) {
    if(!resolved()) ierror("Attempted to output unresolved instruction.");
    buf[location]=op;
    for(int i=0;i<parameters.size();i++) { buf[location+i+1] = parameters[i]; }
    if(next) next->output(buf);
  }
  
  int padd(uint8_t param) { parameters.push_back(param); }
  int padd16(uint16_t param) { padd(param>>8); padd(param & 0xFF); }
};

/* For manipulating instruction chains */
Instruction* chain_end(Instruction* chain) 
{ return (chain->next==NULL) ? chain : chain_end(chain->next); } 
Instruction* chain_start(Instruction* chain) 
{ return (chain->prev==NULL) ? chain : chain_start(chain->prev); } 
Instruction* chain_i(Instruction** chain, Instruction* newi) {
  if(*chain) { (*chain)->next=newi; } newi->prev=*chain;
  return *chain=chain_end(newi);
}
void chain_insert(Instruction* after, Instruction* insert) {
  if(after->next) after->next->prev=chain_end(insert);
  chain_end(insert)->next=after->next; insert->prev=after; after->next=insert;
}

string wrap_print(ostream* out, string accumulated, string newblock, int len) {
  if(len==-1 || accumulated.size()+newblock.size()<=len) {
    return accumulated + newblock;
  } else if(len==0) { // never accumulate
    *out << newblock; return "";
  } else { // gone past wrap boundary
    *out << accumulated << endl; return "  "+newblock;
  }
}

// walk through instructions, printing:
void print_chain(Instruction* chain, ostream* out, int compactness=0) {
  int code_len=0, line_len = (compactness ? (compactness==1 ? 70 : -1) : 0);
  string header = "uint8_t script[] = {"; 
  string line = wrap_print(out,"",header+(compactness ? " " : "\n  "),line_len);
  while(chain) {
    code_len = (chain->location + chain->size());
    string block = chain->to_str(); chain = chain->next; 
    if(chain) block += (compactness ? ", " : ",\n  ");
    line = wrap_print(out,line,block,line_len);
  }
  line = wrap_print(out,line," };",line_len);
  *out << line << endl << "uint16_t script_len = " << code_len << ";" << endl;
}


class Global : public Instruction {
public:
  int index;
  Global(OPCODE op) : Instruction(op) { index = -1; }
  virtual bool isA(string c){return c=="Global"||Instruction::isA(c);}
};

class iDEF_VM : public Instruction {
public:
  int export_len, n_exports, n_globals, n_states, max_stack, max_env;
  iDEF_VM() : Instruction(DEF_VM_OP) 
  { export_len=n_exports=n_globals=n_states=max_stack=max_env=-1;}
  virtual bool isA(string c){return c=="iDEF_VM"||Instruction::isA(c);}
  bool resolved() { 
    return export_len>=0 && n_exports>=0 && n_globals>=0 && n_states>=0 
             && max_stack>=0 && max_env>=0 && Instruction::resolved(); 
  }
  virtual void output(uint8_t* buf) {
    padd(export_len); padd(n_exports); padd16(n_globals); padd(n_states);
    padd16(max_stack+1); padd(max_env); // +1 for enclosing function call
    Instruction::output(buf);
  }
  int size() { return 9; }
};

 // DEF_FUN_k_OP, DEF_FUN_OP, DEF_FUN16_OP
class iDEF_FUN : public Global {
public:
  Instruction* ret;
  int fun_size;
  iDEF_FUN() : Global(DEF_FUN_OP) { ret=NULL; fun_size=-1; }
  virtual bool isA(string c){return c=="iDEF_FUN"||Global::isA(c);}
  bool resolved() { return fun_size>=0 && Instruction::resolved(); }
  int size() { return (fun_size<0) ? -1 : Instruction::size(); }
};

// DEF_TUP_OP, DEF_VEC_OP, DEF_NUM_VEC_OP, DEV_NUM_VEC_k_OP
class iDEF_TUP : public Global {
public:
  int size;
  iDEF_TUP(int size,bool literal=false) : Global(DEF_TUP_OP) { 
    this->size=size;
    if(!literal) { // change to DEF_NUM_VEC_OP
      stack_delta = 0;
      if(size <= MAX_DEF_NUM_VEC_OPS) { op = DEF_NUM_VEC_OP + size;
      } else if(size < 256) { op = DEF_NUM_VEC_OP; padd(size);
      } else ierror("Tuple too large: "+i2s(size)+" > 255");
    } else { // keep as DEF_TUP_OP
      stack_delta = -size;
      if(size < 256) { padd(size);
      } else ierror("Tuple too large: "+i2s(size)+" > 255");
    }
  }
  virtual bool isA(string c){return c=="iDEF_TUP"||Global::isA(c);}
};

class iLET : public Instruction { // LET_OP, LET_k_OP
public:
  Instruction* pop;
  set<Instruction*> usages;
  iLET() : Instruction(LET_1_OP,1) { pop=NULL; }
  virtual bool isA(string c){return c=="iLET"||Instruction::isA(c);}
  bool resolved() { return pop!=NULL && Instruction::resolved(); }
};

// REF_OP, REF_k_OP, GLO_REF16_OP, GLO_REF_OP, GLO_REF_k_OP
class Reference : public Instruction {
public:
  CompilationElement* store; // either an iLET or a Global
  int offset; bool vec_op;
  Reference(CompilationElement* store) : Instruction(GLO_REF_OP) { 
    bool global = store->isA("Global"); // else is a let
    this->store=store; offset=-1; vec_op=false;
    if(!global) { op=REF_OP; ((iLET*)store)->usages.insert(this); }
  }
  // vector op form
  Reference(OPCODE op, CompilationElement* store) : Instruction(op) { 
    if(!store->isA("Global")) ierror("Vector reference to non-global");
    this->store=store; offset=-1; padd(255); vec_op=true;
  }
  virtual bool isA(string c){return c=="Reference"||Instruction::isA(c);}
  bool resolved() { return offset>=0 && Instruction::resolved(); }
  int size() { return (offset<0) ? -1 : Instruction::size(); }
  void set_offset(int o) {
    offset = o;
    if(vec_op) {
      if(o < 256) parameters[0] = o;
      else ierror("Vector reference too large: "+i2s(o));
    } else if(store->isA("Global")) {
      parameters.clear();
      if(o < MAX_GLO_REF_OPS) { op = GLO_REF_0_OP + o;
      } else if(o < 256) { op = GLO_REF_OP; padd(o);
      } else if(o < 65536) { op = GLO_REF16_OP; padd16(o);
      } else ierror("Global reference too large: "+i2s(o));
    } else {
      parameters.clear();
      if(o < MAX_REF_OPS) { op = REF_0_OP + o;
      } else if(o < 256) { op = REF_OP; padd(o);
      } else ierror("Environment reference too large: "+i2s(o));
    }
  }
};

/*****************************************************************************
 *  PROPAGATOR                                                               *
 *****************************************************************************/

class InstructionPropagator : public CompilationElement {
 public:
  // behavior variables
  int verbosity;
  int loop_abort; // # equivalent passes through worklist before assuming loop
  // propagation work variables
  set<Instruction*> worklist_i;
  bool any_changes;
  Instruction* root;
  
  InstructionPropagator(int abort=10) { loop_abort=abort; }
  bool propagate(Instruction* chain); // act on worklist until empty
  virtual void preprop() {} virtual void postprop() {} // hooks
  // action routines to be filled in by inheritors
  virtual void act(Instruction* i) {}
  // note_change: adds neighbors to the worklist
  void note_change(Instruction* i);
 private:
  void queue_nbrs(Instruction* i, int marks=0);
};

CompilationElement* isrc; set<CompilationElement*> iqueued;
void InstructionPropagator::note_change(Instruction* i) 
{ iqueued.clear(); any_changes=true; isrc=i; queue_nbrs(i); }
void InstructionPropagator::queue_nbrs(Instruction* i, int marks) {
  if(i->prev) worklist_i.insert(i->prev); // sequence neighbors
  if(i->next) worklist_i.insert(i->next);
  set<Instruction*>::iterator j=i->dependents.begin(); // any asking for wakeup
  for( ; j!=i->dependents.end(); j++) worklist_i.insert(*j);
 }

bool InstructionPropagator::propagate(Instruction* chain) {
  if(verbosity>=1) *cpout << "Executing analyzer " << to_str() << endl;
  any_changes=false; root = chain_start(chain);
  // initialize worklists
  worklist_i.clear(); 
  while(chain) { worklist_i.insert(chain); chain=chain->next; }
  // walk through worklists until empty
  preprop();
  int steps_remaining = loop_abort*(worklist_i.size());
  while(steps_remaining>0 && !worklist_i.empty()) {
    // each time through, try executing one from each worklist
    if(!worklist_i.empty()) {
      Instruction* i = *worklist_i.begin(); worklist_i.erase(i); 
      act(i); steps_remaining--;
    }
  }
  if(steps_remaining<=0) ierror("Aborting due to apparent infinite loop.");
  postprop();
  return any_changes;
}



class StackEnvSizer : public InstructionPropagator {
public:
  map<Instruction*,int> stack_height;
  map<Instruction*,int> env_height;
  StackEnvSizer(ProtoKernelEmitter* parent,Args* args) 
  { verbosity = parent->verbosity; }
  void print(ostream* out=0) { *out<<"StackEnvSizer"; }
  void preprop() { stack_height.clear(); env_height.clear(); }
  void postprop() {
    Instruction* chain=root;
    int max_env=0, max_stack=0;
    //print_chain(chain,cpout,false);
    string ss=" Stack heights: ", es=" Env heights:   ";
    while(chain) { 
      if(!stack_height.count(chain) || !env_height.count(chain))
        return; // wasn't able to entirely resolve
      max_stack = MAX(max_stack,stack_height[chain]);
      max_env = MAX(max_env,env_height[chain]);
      ss += i2s(stack_height[chain])+" "; es += i2s(env_height[chain])+" ";
      chain=chain->next;
    }
    if(verbosity>=2) *cpout << ss << endl << es << endl;
    int final = stack_height[chain_end(root)];
    if(final) ierror("Stack resolves to non-zero height: "+i2s(final));
    iDEF_VM* dv = (iDEF_VM*)root;
    if(dv->max_stack!=max_stack || dv->max_env!=max_env) {
      dv->max_stack=max_stack; dv->max_env=max_env;
      any_changes=true;
    } else {
      any_changes=false;
    }
  }
  void maybe_set_stack(Instruction* i,int h) {
    if(!stack_height.count(i) || stack_height[i]!=h)
      { stack_height[i]=h; note_change(i); }
  }
  void maybe_set_env(Instruction* i,int h) {
    if(!env_height.count(i) || env_height[i]!=h) 
      { env_height[i]=h; note_change(i); }
  }
  void act(Instruction* i) {
    // stack heights:
    if(!i->prev) { maybe_set_stack(i,i->stack_delta);
    } else if(stack_height.count(i->prev)) {
      maybe_set_stack(i,stack_height[i->prev] + i->prev->stack_delta);
    }
    // and also the case for function calls...

    // env heights:
    if(!i->prev) { maybe_set_env(i,i->env_delta);
    } else if(env_height.count(i->prev)) {
      maybe_set_env(i,env_height[i->prev] + i->prev->env_delta);
    }
    // and also the case for function calls...

  }
};

class InsertLetPops : public InstructionPropagator {
public:
  InsertLetPops(ProtoKernelEmitter* parent,Args* args) 
  { verbosity = parent->verbosity; }
  void print(ostream* out=0) { *out<<"InsertLetPops"; }
  void act(Instruction* i) {
    if(i->isA("iLET")) { iLET* l = (iLET*)i;
      if(l->pop!=NULL) return; // don't do it when pops are resolved
      vector<iLET*> sources; sources.push_back(l);
      if(verbosity>=2) *cpout << "Considering a LET";
      vector<set<Instruction*> > usages;
      usages.push_back(l->usages); //1 per src
      Instruction* pointer = l->next;
      while(!usages.empty()) {
        if(verbosity>=2) *cpout << ".";
        while(sources.size()>usages.size()) sources.pop_back(); // cleanup...
        if(pointer==NULL) ierror("Couldn't find all usages of let");
        if(pointer->isA("iLET")) { // add subs in
          if(verbosity>=2) *cpout << "\n Adding sub LET";
          iLET* sub = (iLET*)pointer; sources.push_back(sub);
          usages.push_back(sub->usages);
        } else if(pointer->isA("Reference")) { // it's somebody's reference?
          if(verbosity>=2) *cpout << "\n Searching for reference... offset = ";
          for(int j=0;j<usages.size();j++) {
            if(usages[j].count(pointer)) {
              if(verbosity>=2) *cpout << (sources.size()-1-j) << " ";
              ((Reference*)pointer)->set_offset(sources.size()-1-j);
              usages[j].erase(pointer);
              break;
            }
          }
          // trim any empty usages on top of the stack
          while(usages.size() && usages[usages.size()-1].empty()) {
            if(verbosity>=2) *cpout << "\n Popping a LET";
            usages.pop_back();
          }
        }
        if(!usages.empty()) pointer=pointer->next;
      }
      if(verbosity>=2) *cpout << "\n Adding pop of size "<<sources.size()<<"\n";
      if(sources.size()<=MAX_LET_OPS) {
        l->pop=new Instruction(POP_LET_OP+sources.size());
      } else {
        l->pop=new Instruction(POP_LET_OP); l->pop->padd(sources.size());
      }
      l->pop->env_delta = -sources.size();
      for(int i=0;i<sources.size();i++) sources[i]->pop = l->pop;
      if(verbosity>=2) *cpout << " Completed LET resolution\n";
      chain_insert(pointer,l->pop);
    }
  }
};

class ResolveISizes : public InstructionPropagator {
public:
  ResolveISizes(ProtoKernelEmitter* parent,Args* args) 
  { verbosity = parent->verbosity; }
  void print(ostream* out=0) { *out<<"ResolveISizes"; }
  void act(Instruction* i) {
    if(i->isA("iDEF_FUN")) {
      iDEF_FUN *df = (iDEF_FUN*)i; 
      bool ok=true; int size=1; // return's size
      Instruction* j = df->next;
      while(j!=df->ret) {
        if(j->size()>=0) size += j->size(); else ok=false;
        j=j->next;  if(!j) ierror("DEF_FUN_OP can't find matching RET_OP");
      }
      if(size>0 && df->fun_size != size) {
        if(verbosity>=2) *cpout << " Fun size is " << size << endl;
        df->fun_size=size; df->parameters.clear();
        // now adjust the op
        if(df->fun_size>1 && df->fun_size<=MAX_DEF_FUN_OPS) 
          df->op=(DEF_FUN_2_OP+(df->fun_size-2));
        else if(df->fun_size < 256)
          { df->op = DEF_FUN_OP; df->padd(df->fun_size); }
        else if(df->fun_size < 65536) 
          { df->op = DEF_FUN16_OP; df->padd16(df->fun_size); }
        else ierror("Function size too large: "+df->fun_size);
        note_change(i);
      }
    }
    if(i->isA("Reference")) {
      Reference* r = (Reference*)i;
      if(r->offset==-1 && r->store->isA("Global") && 
         ((Global*)r->store)->index >= 0) {
        if(verbosity>=2) 
          *cpout << " Global index to " << r->store->to_str()
                 << " is " << ((Global*)r->store)->index << endl;
        r->set_offset(((Global*)r->store)->index);
        note_change(i);
      }
    }
  }
};

class ResolveLocations : public InstructionPropagator {
public:
  // Note: a possible concern: it's theoretically possible
  // to end up in an infinite loop when location is being nudged
  // such that reference lengths change, which could change op size
  // and therefore location...
  ResolveLocations(ProtoKernelEmitter* parent,Args* args) 
  { verbosity = parent->verbosity; }
  void print(ostream* out=0) { *out<<"ResolveLocations"; }
  void maybe_set_location(Instruction* i, int l) {
    if(i->location != l) { i->location=l; note_change(i); }
  }
  void maybe_set_index(Instruction* i, int l) {
    g_max = MAX(g_max,l+1);
    if(((Global*)i)->index != l) { ((Global*)i)->index=l; note_change(i); }
  }
  void act(Instruction* i) {
    if(!i->prev) { maybe_set_location(i,0); }
    if(i->prev && i->prev->location>=0 && i->prev->size()>=0)
      { maybe_set_location(i,i->prev->location + i->prev->size()); }
    if(i->isA("Global")) {
      Instruction *ptr = i->prev; // find previous global...
      while(ptr && !ptr->isA("Global")) { ptr = ptr->prev; }
      Global* g_prev = (Global*)ptr;
      if(g_prev) {
        ptr->dependents.insert(i); // make sure we'll get triggered when it sets
        if(g_prev->index!=-1) maybe_set_index(i,g_prev->index+1); 
      } else { maybe_set_index(i,0); }
    }
  }
  int g_max; // highest global index seen
  void preprop() { g_max = 0; }
  void postprop() {
    iDEF_VM* dv = (iDEF_VM*)root;
    if(dv->n_globals!=g_max) { dv->n_globals=g_max; any_changes|=true; }
  }
};

// counts states, exports
class ResolveState : public InstructionPropagator {
public:
  int n_states, n_exports, export_len;
  bool unresolved;
  ResolveState(ProtoKernelEmitter* parent,Args* args) 
  { verbosity = parent->verbosity; }
  void print(ostream* out=0) { *out<<"ResolveState"; }
  void preprop(){ unresolved=false; n_states=n_exports=export_len=0; }
  void postprop() {
    if(unresolved) return; // only set VM state if all is resolved
    iDEF_VM* dv = (iDEF_VM*)root;
    dv->n_states=n_states; dv->n_exports=n_exports; dv->export_len=export_len;
  }
  void act(Instruction* i) {
    // count up states, etc. here & resolve their pointings
  }
};

class CheckResolution : public InstructionPropagator {
public:
  CheckResolution(ProtoKernelEmitter* parent) { verbosity = parent->verbosity; }
  void print(ostream* out=0) { *out<<"CheckResolution"; }
  void act(Instruction* i) { 
    if(!i->resolved()) ierror("Instruction resolution failed for "+i->to_str());
  }
};


/*****************************************************************************
 *  STATIC/LOCAL TYPE CHECKER                                                *
 *****************************************************************************/
// ensures that all 
class Emittable {
 public:
  // concreteness of fields comes from their types
  static bool acceptable(Field* f) { return acceptable(f->range); }
  // concreteness of types
  static bool acceptable(ProtoType* t) { 
    if(t->isA("ProtoScalar")) { return true;
    } else if(t->isA("ProtoSymbol")) { return true;
    } else if(t->isA("ProtoTuple")) { 
      ProtoTuple* tp = dynamic_cast<ProtoTuple*>(t);
      if(!tp->bounded) return false;
      for(int i=0;i<tp->types.size();i++)
        if(!acceptable(tp->types[i])) return false;
      return true;
    } else if(t->isA("ProtoLambda")) { 
      ProtoLambda* tl = dynamic_cast<ProtoLambda*>(t); 
      return acceptable(tl->op);
    } else return false; // all others, including ProtoField
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


class CheckEmittableType : public Propagator {
 public:
  CheckEmittableType(ProtoKernelEmitter* parent) : Propagator(true,false)
  { verbosity = parent->verbosity; }
  virtual void print(ostream* out=0) { *out << "CheckEmittableType"; }
  virtual void act(Field* f) {
    if(!Emittable::acceptable(f->range))
      ierror(f,"Type is not resolved to emittable form: "+f->to_str());
  }
};



/*****************************************************************************
 *  EMITTER PROPER                                                           *
 *****************************************************************************/

typedef union { flo val; uint8_t bytes[4]; } FLO_BYTES;
Instruction* ProtoKernelEmitter::literal_to_instruction(ProtoType* l) {
  if(l->isA("ProtoScalar")) {
    float val = dynamic_cast<ProtoScalar*>(l)->value;
    if(val == (int)val) { // try to use integer
      if (val >= 0 && val < MAX_LIT_OPS) {
        return new Instruction(LIT_0_OP+(uint8_t)val);
      } else if (val >= 0 && val < 128) { 
        Instruction* i = new Instruction(LIT8_OP); i->padd(val); return i;
      } else if (val >= 0 && val < 65536) {
        Instruction* i = new Instruction(LIT16_OP); i->padd16(val); return i;
      }
    }
    // otherwise, is a floating literal
    FLO_BYTES f; f.val=val;
    Instruction* i = new Instruction(LIT_FLO_OP);
    i->padd(f.bytes[0]); i->padd(f.bytes[1]); i->padd(f.bytes[2]);
    i->padd(f.bytes[3]); return i;
  } else if(l->isA("ProtoTuple")) {
    ProtoTuple* t = dynamic_cast<ProtoTuple*>(l);
    if(!t->bounded) ierror("Cannot emit unbounded literal tuple "+t->to_str());
    if(t->types.size()==0) return new Instruction(NUL_TUP_OP); // special case
    // declare a global tup initialized to the right values
    Instruction* deftup=NULL;
    for(int i=0;i<t->types.size();i++) 
      chain_i(&deftup,literal_to_instruction(t->types[i]));
    chain_i(&deftup,new iDEF_TUP(t->types.size(),true));
    chain_i(&end,chain_start(deftup)); // add to global program
    // make a global reference to it
    return new Reference(deftup);
    //return new Reference();
  } else {
    ierror("Don't know how to emit literal: "+l->to_str());
  }
}

// adds a tuple to the global declarations, then references it in vector op i
Instruction* ProtoKernelEmitter::vec_op_store(ProtoType* t) {
  ProtoTuple* tt = dynamic_cast<ProtoTuple*>(t);
  return chain_i(&end,new iDEF_TUP(tt->types.size()));
}

// takes OperatorInstance because sometimes the operator type depends on it
Instruction* ProtoKernelEmitter::primitive_to_instruction(OperatorInstance* oi){
  Primitive* p = (Primitive*)oi->op;
  ProtoType* otype = oi->output->range; bool tuple = otype->isA("ProtoTuple"); 
  if(primitive2op.count(p->name)) { // plain ops
    if(tuple) return new Reference(primitive2op[p->name],vec_op_store(otype));
    else return new Instruction(primitive2op[p->name]);
  } else if(sv_ops.count(p->name)) { // scalar/vector paired ops
    bool anytuple=tuple; // op switch happens if *any* input is non-scalar
    for(int i=0;i<oi->inputs.size();i++) 
      anytuple|=oi->inputs[i]->range->isA("ProtoTuple");
    OPCODE c = anytuple ? sv_ops[p->name].second : sv_ops[p->name].first;
    // now add ops: possible multiple if n-ary
    int n_copies = (p->signature->rest_input ? oi->inputs.size()-1 : 1);
    Instruction *chain = NULL;
    for(int i=0;i<n_copies;i++) {
      if(tuple && !(p->name=="max" || p->name=="min"))
        chain_i(&chain,new Reference(c,vec_op_store(otype)));
      else chain_i(&chain,new Instruction(c));
    }
    return chain_start(chain);
  } else if(p->name=="/") { // special handling until VDIV is added
    Instruction *chain = NULL;
    for(int i=0;i<oi->inputs.size()-2;i++) // multiply the divisors together
      chain_i(&chain,new Instruction(MUL_OP));
    if(!tuple) { chain_i(&chain,new Instruction(DIV_OP));
    } else { // multiply by 1/divisor
      chain_i(&chain,new Instruction(LET_2_OP,2));
      chain_i(&chain,new Instruction(LIT_1_OP));
      chain_i(&chain,new Instruction(REF_0_OP));
      chain_i(&chain,new Instruction(DIV_OP));
      chain_i(&chain,new Instruction(REF_1_OP));
      chain_i(&chain,new Reference(VMUL_OP,vec_op_store(otype)));
      chain_i(&chain,new Instruction(POP_LET_2_OP,-2));
    }
    return chain_start(chain);
  } else if(p->name=="tup") {
    Instruction* i = new Reference(TUP_OP,vec_op_store(otype)); 
    i->stack_delta = 1-oi->inputs.size(); i->padd(oi->inputs.size());
    return i;
  } else {
    ierror("Don't know how to convert op to instruction: "+p->to_str());
  }
}



void ProtoKernelEmitter::load_ops(string name, NeoCompiler* parent) {
  ifstream* f = parent->proto_path.find_in_path(name);
  if(f==NULL) { compile_error("Can't find file '"+name+"'"); return; }
  SExpr* exp = read_sexpr(name,f); if(!exp) { compiler_error=true; return; }
  if(!exp->isList()) { compile_error(exp,"Op declaration not a list"); return; }
  SE_List* sl = (SE_List*)exp;
  for(int i=0;i<sl->len();i++) {
    SE_List* op = (SE_List*)(*sl)[i];
    if(!(*sl)[i]->isList() || op->len()<2||op->len()>3 || !(*op)[0]->isSymbol()
       || (op->len()==3 && !(*op)[2]->isSymbol())) { 
      compile_error((*sl)[i],"Op not formatted (name stack-delta [primitive])");
      return;
    }
    opnames[i] = ((SE_Symbol*)(*op)[0])->name;
    if((*op)[1]->isScalar()) 
      op_stackdeltas[i] = ((SE_Scalar*)(*op)[1])->value;
    else if(((SE_Symbol*)(*op)[1])->name=="variable")
      op_stackdeltas[i] = 7734; // give variables a fixed bogus number
    else compile_error((*op)[1],"Unknown op stack-delta: "+(*op)[1]->to_str());
    if(op->len()==3) primitive2op[((SE_Symbol*)(*op)[2])->name] = i;
  }

  // now add the special-case ops
  sv_ops["+"] = make_pair(ADD_OP,VADD_OP);
  sv_ops["-"] = make_pair(SUB_OP,VSUB_OP);
  sv_ops["*"] = make_pair(MUL_OP,VMUL_OP);
  //sv_ops["/"] = make_pair(DIV_OP,VDIV_OP);
  sv_ops["<"] = make_pair(LT_OP,VLT_OP);
  sv_ops["<="] = make_pair(LTE_OP,VLTE_OP);
  sv_ops[">"] = make_pair(GT_OP,VGT_OP);
  sv_ops[">="] = make_pair(GTE_OP,VGTE_OP);
  sv_ops["="] = make_pair(EQ_OP,VEQ_OP);
  sv_ops["max"] = make_pair(MAX_OP,VMAX_OP);
  sv_ops["min"] = make_pair(MIN_OP,VMIN_OP);
  sv_ops["mux"] = make_pair(MUX_OP,VMUX_OP);
}

ProtoKernelEmitter::ProtoKernelEmitter(NeoCompiler* parent, Args* args) {
  // set global variables
  this->parent=parent;
  is_dump_hex = args->extract_switch("--hexdump");
  print_compact = (args->extract_switch("--emit-compact") ? 2 :
                   (args->extract_switch("--emit-semicompact") ? 1 : 0));
  verbosity = args->extract_switch("--emitter-verbosity")?args->pop_number():0;
  max_loops=args->extract_switch("--emitter-max-loops")?args->pop_number():10;
  paranoid = args->extract_switch("--emitter-paranoid");
  // load operation definitions
  string name = "core.ops"; load_ops(name,parent); terminate_on_error();
  // setup rule collection
  rules.push_back(new InsertLetPops(this,args));
  rules.push_back(new ResolveISizes(this,args));
  rules.push_back(new ResolveLocations(this,args));
  rules.push_back(new StackEnvSizer(this,args));
  rules.push_back(new ResolveState(this,args));
  // program starts empty
  start=end=NULL;
}

/* this walks a DFG in order as follows: 
   - producer before consumers
   - consumers in order
*/
Instruction* ProtoKernelEmitter::tree2instructions(Field* f) {
  if(memory.count(f)) { return new Reference(memory[f]); }
  OperatorInstance* oi = f->producer; Instruction* chain = NULL;
  // first, get all the inputs
  for(int i=0;i<oi->inputs.size();i++) 
    chain_i(&chain,tree2instructions(oi->inputs[i]));
  // second, add the operation
  if(oi->op->isA("Primitive")) {
    chain_i(&chain,primitive_to_instruction(oi));
  } else if(oi->op->isA("Literal")) { 
    chain_i(&chain,literal_to_instruction(((Literal*)oi->op)->value)); 
  } else { // also CompoundOp, Parameter
    ierror("Don't know how to emit instruction for "+oi->op->to_str());
  }
  // finally, put the result in the appropriate locations
  if(f->selectors.size())
    ierror("Restrictions not all compiled out for: "+f->to_str());
  if(f->consumers.size()>=2) { // need a let to contain this
    memory[f] = chain_i(&chain, new iLET());
    chain_i(&chain,new Reference(memory[f])); // and we got here by a ref...
  }
  return chain_start(chain);
}

Instruction* ProtoKernelEmitter::dfg2instructions(DFG* g) {
  set<Field*> minima;
  for(set<Field*>::iterator i=g->edges.begin();i!=g->edges.end();i++) 
    if(!(*i)->consumers.size()) minima.insert(*i);
  
  //cout << "Minima identified: " << v2s(&minima) << endl;
  iDEF_FUN *fnstart = new iDEF_FUN(); Instruction *chain=fnstart;
  for(set<Field*>::iterator i=minima.begin();i!=minima.end();i++)
    chain_i(&chain, tree2instructions(*i));
  if(minima.size()>1) { // needs an all
    Instruction* all = new Instruction(ALL_OP);
    if(minima.size()>=256) ierror("Too many minima: "+minima.size());
    all->stack_delta = -(minima.size()-1); all->padd(minima.size());
    chain_i(&chain, all);
  }
  chain_i(&chain, fnstart->ret = new Instruction(RET_OP));
  return fnstart;
}

string hexbyte(uint8_t v) {
  string hex = "0123456789ABCDEF";
  string out = "xx"; out[0]=hex[v>>4]; out[1]=hex[v & 0xf]; return out;
}

uint8_t* ProtoKernelEmitter::emit_from(DFG* g, int* len) {
  CheckEmittableType echecker(this); echecker.propagate(g);

  if(verbosity>=1) *cpout << "Linearizing DFG to instructions...\n";
  start = end = new iDEF_VM(); // start of every script
  map<Operator*,set<OperatorInstance*> >::iterator i=g->funcalls.begin();
  for( ; i!=g->funcalls.end(); i++) // translate each function
    chain_i(&end,dfg2instructions(((CompoundOp*)((*i).first))->body));
  chain_i(&end,dfg2instructions(g)); // next the main
  chain_i(&end,new Instruction(EXIT_OP)); // add the end op
  if(verbosity>=2) print_chain(start,cpout,2);
  
  if(verbosity>=1) *cpout << "Resolving unknowns in instruction sequence...\n";
  for(int i=0;i<max_loops;i++) {
    bool changed=false;
    for(int j=0;j<rules.size();j++) {
      changed |= rules[j]->propagate(start); terminate_on_error();
    }
    if(!changed) break;
    if(i==(max_loops-1))
      compile_warn("Analyzer giving up after "+i2s(max_loops)+" loops");
  }
  CheckResolution rchecker(this); rchecker.propagate(start);
  
  // finally, output
  if(verbosity>=1) *cpout << "Outputting final instruction sequence...\n";
  *len= (end->location + end->size());
  uint8_t* buf = (uint8_t*)calloc(*len,sizeof(uint8_t));
  start->output(buf);
  
  if(parent->is_dump_code) print_chain(start,cpout,print_compact);

  if(is_dump_hex) {
    for(int i=0;i<*len;i++) // dump lines of 25 bytes each (75 chars)
      { *cpout << hexbyte(buf[i]) << " "; if((i % 25)==24) *cpout << endl; } 
    if(*len % 25) *cpout << endl; // close final line if needed
  }
  
  return buf;
}

// How will the emitter work:
// There are template mappings for;
// Literal: -> LIT_k_OP, LIT8_OP, LIT16_OP, LIT_FLO_OP
// Primitive: 
// Compound Op:
// Parameter -> REF_k_OP, REF_OP
// Output -> RET_OP
// field w. multiple consumers -> LET_k_OP, LET_OP; end w. POP_LET_k_OP, POP_LET_OP

// What about...? REF, 
// GLO_REF is used for function calls...
// DEF_FUN, IF (restrict) emission

/* LINEARIZING:
   let's give each element a number in the order it's created in;
   then, we'll have the set be in that order, so there's a stable
   order in which non-ordered program fragments are executed
     i.e. (all (set-dt 3) (green 2) 7)
   We need to figure out how to linearize a walk through a branching
   sequence: i.e. (let ((x 5)) (set-dt x) (+ 1 x))

  We are guaranteed that it's a partially ordered graph.
  Requirement: label all nodes "i" s.t. i={1...n}, no repeats, 
    no input > output

  1. segment the program into independent computations; order by elt#
  2. in each independent computation, find set of bottoms & start w. min elt#
  3. walk tree to linearize: ins before outs, out in rising elt# order

 */
