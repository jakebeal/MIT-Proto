/* ProtoKernel code emitter
Copyright (C) 2009, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// This turns a Proto program representation into ProtoKernel bytecode to
// execute it.

#include "config.h"
#include "compiler.h"
#include "proto_opcodes.h"

map<int,string> opnames;
map<string,int> primitive2op;
map<int,int> op_stackdeltas;

/// instructions like SQRT_OP that have no special rules or pointers
struct Block;
struct Instruction : public CompilationElement { reflection_sub(Instruction,CE);
  Instruction *next,*prev; // sequence links
  Block* container;
  int location; // -1 = unknown
   // instructions "neighboring" this one
  set<Instruction*, CompilationElement_cmp> dependents;
  
  OPCODE op;
  vector<uint8_t> parameters; // values consumed after op
  int stack_delta; // change in stack size following this instruction
  int env_delta; // change in environment size following this instruction
  Instruction(OPCODE op, int ed=0) {
    this->op=op; stack_delta=op_stackdeltas[op]; env_delta=ed; 
    location=-1; next=prev=NULL; container = NULL;
  }
  virtual void print(ostream* out=0) {
    *out << (opnames.count(op) ? opnames[op] : "<UNKNOWN OP>");
    //*out << "[" << location << "]";
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
  
  virtual int start_location() { return location; }
  virtual int next_location() { 
    if(location==-1 || size()==-1) return -1;
    else return location+size();
  }
  virtual void set_location(int l) { location = l; } 
  virtual int net_env_delta() { return env_delta; }
  virtual int max_env_delta() { return MAX(0,env_delta); }
  virtual int net_stack_delta() { return stack_delta; }
  virtual int max_stack_delta() { return MAX(0,stack_delta); }

  int padd(uint8_t param) { parameters.push_back(param); }
  int padd16(uint16_t param) { padd(param>>8); padd(param & 0xFF); }
};

/// A block is a sequence of instructions
struct Block : public Instruction { reflection_sub(Block,Instruction);
  Instruction* contents;
  Block(Instruction* chain);
  virtual void print(ostream* out=0);
  virtual int size() { 
    Instruction* ptr = contents; int s=0;
    while(ptr){ if(ptr->size()==-1){return -1;} s+=ptr->size(); ptr=ptr->next; }
    return s;
  }
  virtual bool resolved() { 
    Instruction* ptr = contents; 
    while(ptr) { if(!ptr->resolved()) return false; ptr=ptr->next; }
    return true;
  }
  virtual void output(uint8_t* buf) {
    Instruction* ptr = contents; while(ptr) { ptr->output(buf); ptr=ptr->next; }
    if(next) next->output(buf);
  }

  virtual int net_env_delta() { 
    env_delta = 0;
    Instruction* ptr = contents;
    while(ptr) { env_delta+=ptr->net_env_delta(); ptr=ptr->next; }
    return env_delta;
  }
  virtual int max_env_delta() { 
    int delta = 0, max_delta = 0;
    Instruction* ptr = contents;
    while(ptr) { 
      max_delta = MAX(max_delta,delta+ptr->max_env_delta());
      delta+=ptr->net_env_delta(); ptr=ptr->next;
    }
    return max_delta;
  }
  virtual int net_stack_delta() {
    stack_delta = 0;
    Instruction* ptr = contents;
    while(ptr) { stack_delta+=ptr->net_stack_delta(); ptr=ptr->next; }
    return stack_delta;
  }
  virtual int max_stack_delta() {
    int delta = 0, max_delta = 0;
    Instruction* ptr = contents;
    while(ptr) { 
      max_delta = MAX(max_delta,delta+ptr->max_stack_delta());
      delta+=ptr->net_stack_delta(); ptr=ptr->next;
    }
    return max_delta;
  }
};


/* For manipulating instruction chains */
Instruction* chain_end(Instruction* chain) 
{ return (chain->next==NULL) ? chain : chain_end(chain->next); } 
Instruction* chain_start(Instruction* chain) 
{ return (chain->prev==NULL) ? chain : chain_start(chain->prev); } 
Instruction* chain_i(Instruction** chain, Instruction* newi) {
  if(*chain) { 
    (*chain)->next=newi;
    if((*chain)->container) {
      Instruction* p = newi;
      while(p) { 
        p->container = (*chain)->container;
        (*chain)->container->dependents.insert(p);
        p = p->next;
      }
    }
  } 
  newi->prev=*chain;
  return *chain=chain_end(newi);
}
void chain_insert(Instruction* after, Instruction* insert) {
  if(after->next) after->next->prev=chain_end(insert);
  chain_end(insert)->next=after->next; insert->prev=after; after->next=insert;
}
Instruction* chain_split(Instruction* newstart) {
  Instruction* prev = newstart->prev;
  newstart->prev=NULL; if(prev) prev->next=NULL;
  return prev;
}
void chain_delete(Instruction* start,Instruction* end) {
  if(end->next) end->next->prev=start->prev;
  if(start->prev) start->prev->next=end->next;
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

string print_raw_chain(Instruction* chain, string line, int line_len,
                       ostream* out, int compactness, bool recursed=false) {
  while(chain) {
    if(chain->isA("Block")) {
      line = print_raw_chain(((Block*)chain)->contents,line,line_len,
                             out,compactness,true);
      chain = chain->next;
    } else {
      string block = chain->to_str(); chain = chain->next; 
      if(chain || recursed) block += (compactness ? ", " : ",\n  ");
      line = wrap_print(out,line,block,line_len);
    }
  }
  return line;
}

/** 
 * walk through instructions, printing:
 * compactness: 0: one instruction/line, 1: 70-char lines, 2: single line
 */
void print_chain(Instruction* chain, ostream* out, int compactness=0) {
  int line_len = (compactness ? (compactness==1 ? 70 : -1) : 0);
  string header = "uint8_t script[] = {"; 
  string line = wrap_print(out,"",header+(compactness ? " " : "\n  "),line_len);
  line = print_raw_chain(chain,line,line_len,out,compactness);
  *out << wrap_print(out,line," };\n",line_len);
  int code_len = chain_end(chain)->next_location();
  *out<<"uint16_t script_len = " << code_len << ";" << endl;
}

Block::Block(Instruction* chain) : Instruction(-1) { 
  contents = chain_start(chain);
  Instruction* ptr = chain;
  while(ptr){ ptr->container=this; dependents.insert(ptr); ptr=ptr->next; }
}
void Block::print(ostream* out) 
{ *out<<"{"<<print_raw_chain(contents,"",-1,out,3)<<"}"; }

// NoInstruction is a placeholder for deleted bits
struct NoInstruction : public Instruction {
  reflection_sub(NoInstruction,Instruction);
  NoInstruction() : Instruction(-1) {}
  virtual void print(ostream* out=0) { *out << "<No Instruction>"; }
  virtual int size() { return 0; }
  virtual bool resolved() { return start_location()>=0; }
  virtual void output(uint8_t* buf) {
    if(next) next->output(buf);
  }
};

struct Global : public Instruction { reflection_sub(Global,Instruction);
  int index;
  Global(OPCODE op) : Instruction(op) { index = -1; }
};

/// DEF_VM
struct iDEF_VM : public Instruction { reflection_sub(iDEF_VM,Instruction);
  int export_len, n_exports, n_globals, n_states, max_stack, max_env;
  iDEF_VM() : Instruction(DEF_VM_OP) 
  { export_len=n_exports=n_globals=n_states=max_stack=max_env=-1;}
  bool resolved() { 
    return export_len>=0 && n_exports>=0 && n_globals>=0 && n_states>=0 
             && max_stack>=0 && max_env>=0 && Instruction::resolved(); 
  }
  virtual void output(uint8_t* buf) {
    padd(export_len); padd(n_exports); padd16(n_globals); padd(n_states);
    padd16(max_stack+1); padd(max_env); // +1 for enclosing function call
    Instruction::output(buf);
  }
  /*
  virtual void print(ostream* out=0) {
     *out << "VM Definition "
          << "[ export_len:" << export_len
          << ", n_exports:"  << n_exports
          << ", n_globals:"  << n_globals
          << ", n_states:"   << n_states
          << ", max_stack:"  << max_stack
          << ", max_env:"    << max_env
          << " ]";
  }
  */
  int size() { return 9; }
};

/// DEF_FUN_k_OP, DEF_FUN_OP, DEF_FUN16_OP
struct iDEF_FUN : public Global { reflection_sub(iDEF_FUN,Global);
  Instruction* ret;
  int fun_size;
  iDEF_FUN() : Global(DEF_FUN_OP) { ret=NULL; fun_size=-1; }
  bool resolved() { return fun_size>=0 && Instruction::resolved(); }
  int size() { return (fun_size<0) ? -1 : Instruction::size(); }
};

/// DEF_TUP_OP, DEF_VEC_OP, DEF_NUM_VEC_OP, DEV_NUM_VEC_k_OP
struct iDEF_TUP : public Global { reflection_sub(iDEF_TUP,Global);
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
};

/// LET_OP, LET_k_OP
struct iLET : public Instruction { reflection_sub(iLET,Instruction);
  Instruction* pop;
  CEset(Instruction*) usages;
  iLET() : Instruction(LET_1_OP,1) { pop=NULL; }
  bool resolved() { return pop!=NULL && Instruction::resolved(); }
};

/// REF_OP, REF_k_OP, GLO_REF16_OP, GLO_REF_OP, GLO_REF_k_OP
struct Reference : public Instruction { reflection_sub(Reference,Instruction);
  Instruction* store; // either an iLET or a Global
  int offset; bool vec_op;
  Reference(Instruction* store,OI* source) : Instruction(GLO_REF_OP) { 
    attributes["~Ref~Target"] = new CEAttr(source);
    bool global = store->isA("Global"); // else is a let
    this->store=store; store->dependents.insert(this);
    offset=-1; vec_op=false; 
    if(!global) { op=REF_OP; ((iLET*)store)->usages.insert(this); }
  }
  // vector op form
  Reference(OPCODE op, Instruction* store,OI* source) : Instruction(op){ 
    attributes["~Ref~Target"] = new CEAttr(source);
    if(!store->isA("Global")) ierror("Vector reference to non-global");
    this->store=store; store->dependents.insert(this);
    offset=-1; padd(255); vec_op=true;
  }
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

/// IF_OP, IF_16_OP, JMP_OP, JMP_16_OP
struct Branch : public Instruction { reflection_sub(Branch,Instruction);
  Instruction* after_this;
  int offset; bool jmp_op;
  Branch(Instruction* after_this,bool jmp_op=false) : Instruction(IF_OP) {
    this->after_this = after_this; this->jmp_op=jmp_op; offset=-1;
    if(jmp_op) { op=JMP_OP; }
  }
  bool resolved() { return offset>=0 && Instruction::resolved(); }
  void set_offset(int o) {
    offset = o;
    parameters.clear();
    if(o < 256) { op = (jmp_op?JMP_OP:IF_OP); padd(o);
    } else if(o < 65536) { op = (jmp_op?JMP_16_OP:IF_16_OP); padd16(o);
    } else ierror("Branch reference too large: "+i2s(o));
  }
};

/** 
 * Class for handling instructions to call functions
 * (e.g., FUNCALL_0_OP, FUNCALL_1_OP, FUNCALL_OP, etc.)
 */
struct FunctionCall : public Instruction { reflection_sub(FunctionCall,Instruction);
   public:
      CompoundOp* compoundOp;

      /**
       * Creates a new function call instruction.
       */
      FunctionCall(CompoundOp* compoundOpParam)
         : Instruction(FUNCALL_0_OP) {
         compoundOp = compoundOpParam;
         env_delta = getNumParams(compoundOp);
         op = newFunCallInstr(env_delta)->op;
         stack_delta = newFunCallInstr(env_delta)->stack_delta;
      }
   private:
      /**
       * @returns the number of parameters of this function call
       * @TODO: this needs to be smarter about how it handles &rest elements.
       */
      static int getNumParams(CompoundOp* compoundOp) {
         if(!compoundOp->signature->optional_inputs.empty() ||
            compoundOp->signature->rest_input != NULL)
            ierror("Don't know how to handle variable parameter length functions, as in " + ce2s(compoundOp));
         return compoundOp->signature->n_fixed();
      }

      /**
       * Creates a new FUNCALL instruction where index is the number of input
       * parameters to the function.
       */
      static Instruction* newFunCallInstr(int index) {
         Instruction* ret = NULL;
         if(index < 5) ret = new Instruction(FUNCALL_0_OP + index, index);
         else if(index < 256) {ret = new Instruction(FUNCALL_OP, index); ret->padd(index);}
         else ierror("Number of function parameters too large: "+i2s(index));
         return ret;
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
  set<Instruction*, CompilationElement_cmp> worklist_i;
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
  void queue_chain(Instruction* chain);
  void queue_nbrs(Instruction* i, int marks=0);
};

CompilationElement* isrc;
set<CompilationElement*, CompilationElement_cmp> iqueued;
void InstructionPropagator::note_change(Instruction* i) 
{ iqueued.clear(); any_changes=true; isrc=i; queue_nbrs(i); }
void InstructionPropagator::queue_nbrs(Instruction* i, int marks) {
  if(i->prev) worklist_i.insert(i->prev); // sequence neighbors
  if(i->next) worklist_i.insert(i->next);
  if(i->container) worklist_i.insert(i->container); // block
  // plus any asking for wakeup...
  for_set(Instruction*,i->dependents,j) worklist_i.insert(*j);
 }

void InstructionPropagator::queue_chain(Instruction* chain) {
  while(chain) { 
    worklist_i.insert(chain); 
    if(chain->isA("Block")) queue_chain(((Block*)chain)->contents);
    chain=chain->next;
  }
}

bool InstructionPropagator::propagate(Instruction* chain) {
  V1<<"Executing analyzer "<<to_str()<<endl;
  any_changes=false; root = chain_start(chain);
  // initialize worklists
  worklist_i.clear(); queue_chain(chain); 
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
  // height = after instruction; maxes = anytime during
  CEmap(Instruction*,int) stack_height, stack_maxes;
  CEmap(Instruction*,int) env_height, env_maxes;
  StackEnvSizer(ProtoKernelEmitter* parent,Args* args) 
  { verbosity = parent->verbosity; }
  void print(ostream* out=0) { *out<<"StackEnvSizer"; }
  void preprop() { stack_height.clear(); env_height.clear(); }
  void postprop() {
    Instruction* chain=root;
    int max_env=0, max_stack=0;
    string ss=" Stack heights: ", es=" Env heights:   ";
    stack<Instruction*> block_nesting;
    while(true) { 
      if(chain==NULL) {
        if(!block_nesting.empty()) {
          ss+="} "; es+="} ";
          chain=block_nesting.top()->next; block_nesting.pop(); continue;
        } else break;
      } else if(chain->isA("Block")) { // walk through subs
        ss+="{ ";es+="{ ";
        block_nesting.push(chain); chain = ((Block*)chain)->contents; continue;
      } else if(chain->isA("Reference")) { // reference depths in environment:
        Reference* r = (Reference*)chain;
        if(r->store->isA("iLET") && env_height.count(r->store) &&
           env_height.count(r)) {
          int rh = env_height[r], sh = env_height[r->store];
          if(r->offset!=(rh-sh)) {
            V3<<"  Setting let ref offset: "<<rh<<"-"<<sh<<"="<<(rh-sh)<<endl;
            r->set_offset(rh-sh);
          }
        }
      } 
      if(!stack_maxes.count(chain) || !env_maxes.count(chain))
        return; // wasn't able to entirely resolve
      max_stack = MAX(max_stack,stack_maxes[chain]);
      max_env = MAX(max_env,env_maxes[chain]);
      ss += i2s(stack_height[chain])+" "; es += i2s(env_height[chain])+" ";
      chain=chain->next;
    }
    V2 << ss << endl << es << endl;
    int final = stack_height[chain_end(root)];
    if(final) {
       print_chain(chain_start(root), cpout);
       ierror("Stack resolves to non-zero height: "+i2s(final));
    }
    iDEF_VM* dv = (iDEF_VM*)root;
    if(dv->max_stack!=max_stack || dv->max_env!=max_env) {
      dv->max_stack=max_stack; dv->max_env=max_env;
      any_changes=true;
    } else {
      any_changes=false;
    }
  }
  void maybe_set_stack(Instruction* i,int neth,int maxh) {
     V4 << "    Trying to set stack for " << ce2s(i) 
        << " to (" << neth << ", " << maxh << ")" << endl;
     if(!stack_height.count(i) || stack_height[i]!=neth || stack_maxes[i]!=maxh) { 
        stack_height[i]=neth; stack_maxes[i]=maxh; note_change(i); 
        V4 << "    `-Set stack." << endl;
     }
  }
  void maybe_set_env(Instruction* i,int neth, int maxh) {
    if(!env_height.count(i) || env_height[i]!=neth || env_maxes[i]!=maxh) 
      { env_height[i]=neth; env_maxes[i]=maxh; note_change(i); }
  }
  void act(Instruction* i) {
    // stack heights:
    int baseh=-1;
    // base case: i is first instruction.
    if(!i->prev && (!i->container || (i->container && !i->container->prev))) {
      baseh=0;
    }
    // i is the first instruction of a block and inst prior to block has been
    // resolved
    else if(!i->prev && stack_height.count(i->container->prev)) { 
      baseh = stack_height[i->container->prev];
    }
    // i has a previous instruction whose value has been resolved
    else if(i->prev && stack_height.count(i->prev)) {
      baseh = stack_height[i->prev];
    }
    // if the instruction whose value i depends on has been resolved, then
    // resolve i's value
    if(baseh>=0) 
      maybe_set_stack(i,baseh+i->net_stack_delta(),baseh+i->max_stack_delta());
    else
      V4 << "Instruction " << ce2s(i) << " could not be resolved yet." << endl;


    // env heights:
    baseh = -1;
    if(!i->prev && (!i->container || (i->container && !i->container->prev))) {
      baseh=0;
    } else if(!i->prev && env_height.count(i->container->prev)) { 
      baseh = env_height[i->container->prev];
    } else if(i->prev && env_height.count(i->prev)) {
      baseh = env_height[i->prev];
    }
    if(baseh>=0) 
      maybe_set_env(i,baseh+i->net_env_delta(),baseh+i->max_env_delta());
  }
};

class InsertLetPops : public InstructionPropagator {
public:
  InsertLetPops(ProtoKernelEmitter* parent,Args* args) 
  { verbosity = parent->verbosity; }
  void print(ostream* out=0) { *out<<"InsertLetPops"; }

  void insert_pop_set(vector<iLET*> sources,Instruction* pointer) {
    // First, cluster the pops into branch-specific sets
    CEmap(Instruction*,CEset(iLET*)) dest_sets;
    for(int i=0;i<sources.size();i++) {
      // find the last reference to this source
      Instruction* last = NULL;
      for_set(Instruction*,sources[i]->usages,j) {
        if((*j)->marked("~Last~Reference")) { last = *j; break; }
      }
      if(last==NULL)ierror("Trying to pop a let without its last usage marked");
      // is it marked as being in a branch?
      V5 << "  Considering last reference: "<<ce2s(last)<<endl;
      if(last->marked("~Branch~End")) {
        CE* inst = ((CEAttr*)last->attributes["~Branch~End"])->value;
        V5 << "   Branch end ref: "<<ce2s(inst)<<endl;
        dest_sets[(Instruction*)inst].insert(sources[i]);
      } else {
        V5 << "   Default location: "<<ce2s(pointer)<<endl;
        dest_sets[pointer].insert(sources[i]);
      }
    }
    
    // Next, place each set
    for_map(Instruction*, CEset(iLET*), dest_sets, i) {
      // create the pop instruction
      Instruction* pop;
      if(i->second.size()<=MAX_LET_OPS) {
        pop = new Instruction(POP_LET_OP+(i->second.size()));
      } else {
        pop = new Instruction(POP_LET_OP); pop->padd(i->second.size());
      }
      pop->env_delta = -i->second.size();
      // tag it onto the lets in the set
      for_set(iLET*,i->second,j) (*j)->pop = pop;
      // and place the pop at the destination
      string type = (i->first==pointer)?"standard":"branch";
      V3 << "  Inserting "+type+" POP_LET after "<<ce2s(i->first)<<endl;
      chain_insert(i->first,pop);
    }
  }
  
  void act(Instruction* i) {
    if(i->isA("iLET")) { iLET* l = (iLET*)i;
      if(l->pop!=NULL) return; // don't do it when pops are resolved
      vector<iLET*> sources; sources.push_back(l);
      V2 << "Considering a LET";
      vector<set<Instruction*, CompilationElement_cmp> > usages;
      usages.push_back(l->usages); //1 per src
      Instruction* pointer = l->next;
      stack<Instruction*> block_nesting;
      while(!usages.empty()) {
        V3 << ".";
        while(sources.size()>usages.size()) sources.pop_back(); // cleanup...
        if(pointer==NULL) {
          if(block_nesting.empty()) ierror("Couldn't find all usages of let");
          V3 << "^";
          pointer=block_nesting.top()->next; block_nesting.pop(); continue;
        }
        if(pointer->isA("Block")) { // search for references in subs
          V3 << "v";
          block_nesting.push(pointer); pointer = ((Block*)pointer)->contents;
          continue;
        } else if(pointer->isA("iLET")) { // add subs in
          V3 << "\n Adding sub LET";
          iLET* sub = (iLET*)pointer; sources.push_back(sub);
          usages.push_back(sub->usages);
        } else if(pointer->isA("Reference")) { // it's somebody's reference?
          V3 << "\n Found reference...";
          for(int j=0;j<usages.size();j++) {
            if(usages[j].count(pointer)) { 
              usages[j].erase(pointer);
              // mark last references for later use in pop insertion
              if(!usages[j].size()) pointer->mark("~Last~Reference");
              break;
            }
          }
          // trim any empty usages on top of the stack
          while(usages.size() && usages[usages.size()-1].empty()) {
            V3 << "\n Popping a LET";
            usages.pop_back();
          }
        }
        if(!usages.empty()) pointer=pointer->next;
      }
      // Now walk through and pop all the sources, clumping by destination
      V3 << "\n Adding set of pops, size: "<<sources.size()<<"\n";
      insert_pop_set(sources,pointer);
      V3 << " Completed LET resolution\n";
    }
  }
};

class DeleteNulls : public InstructionPropagator {
public:
  DeleteNulls(ProtoKernelEmitter* parent,Args* args) 
  { verbosity = parent->verbosity; }
  void print(ostream* out=0) { *out<<"DeleteNulls"; }
  void act(Instruction* i) {
    if(i->isA("NoInstruction")) {
      V2 << " Deleting NoInstruction placeholder\n";
      chain_delete(i,i);
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
        V2 << " Fun size is " << size << endl;
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
        V2<<" Global index to "<<ce2s(r->store)<<" is "<<
          ((Global*)r->store)->index << endl;
        r->set_offset(((Global*)r->store)->index);
        note_change(i);
      }
    } 
    if(i->isA("Branch")) {
      Branch* b = (Branch*)i; Instruction* target = b->after_this;
      V5<<"    Sizing branch: "<<ce2s(b)<<" over "<<ce2s(target)<<endl;
      if(b->start_location()>=0 && target->start_location()>=0) {
        int diff = target->next_location() - b->next_location();
        if(diff!=b->offset) {
          V2<<" Branch offset to follow "<<ce2s(target)<<" is "<<diff<<endl;
          b->set_offset(diff); note_change(i);
        }
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
  ProtoKernelEmitter* emitter;
  ResolveLocations(ProtoKernelEmitter* parent,Args* args) { 
     verbosity = parent->verbosity; 
     emitter = parent;
  }
  void print(ostream* out=0) { *out<<"ResolveLocations"; }
  void maybe_set_location(Instruction* i, int l) {
    if(i->start_location() != l) { 
      V4 << "   Setting location of "<<ce2s(i)<<" to "<<l<<endl;
      i->set_location(l); note_change(i);
    }
  }
  void maybe_set_index(Instruction* i, int l) {
    g_max = MAX(g_max,l+1);
    if(((Global*)i)->index != l) { 
      V4 << "   Setting index of "<<ce2s(i)<<" to "<<l<<endl;
      ((Global*)i)->index=l; note_change(i);
    }
  }
  void act(Instruction* i) {
    // Base case: set to zero or block-start
    if(!i->prev)
      maybe_set_location(i,(i->container?i->container->start_location():0));
    // Otherwise get it from previous instruction
    if(i->prev && i->prev->next_location()>=0)
      { maybe_set_location(i,i->prev->next_location()); }
    if(i->isA("Global")) {
      Instruction *ptr = i->prev; // find previous global...
      while(ptr && !ptr->isA("Global")) { ptr = ptr->prev; }
      Global* g_prev = (Global*)ptr;
      if(g_prev) {
        ptr->dependents.insert(i); // make sure we'll get triggered when it sets
        if(g_prev->index!=-1) maybe_set_index(i,g_prev->index+1); 
      } else { maybe_set_index(i,0); }
    }
    //if we can resolve the function call to its global index
    if(i->isA("FunctionCall")) {
       if(emitter->globalNameMap.count(((FunctionCall*)i)->compoundOp)) {
          CompoundOp* compoundOp = ((FunctionCall*)i)->compoundOp;
          CompilationElement* ce = emitter->globalNameMap[compoundOp];
          int index = -1;
          if(ce && ce->isA("Global"))
             index = ((Global*)ce)->index;
          if(index >= 0 && i->prev && i->prev->isA("Reference")) {
             ((Reference*)i->prev)->set_offset(index);
             note_change(i);
          }
       }
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
    if(!i->resolved()) {
       print_chain(chain_start(i), cpout);
       ierror("Instruction resolution failed for "+i->to_str());
    }
  }
};


/*****************************************************************************
 *  STATIC/LOCAL TYPE CHECKER                                                *
 *****************************************************************************/
// ensures that all fields are local and concrete
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
      return true; // its fields are checked elsewhere
    } else return false; // generic operator
  }
};


class CheckEmittableType : public IRPropagator {
 public:
  CheckEmittableType(ProtoKernelEmitter* parent) : IRPropagator(true,false)
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
Instruction* ProtoKernelEmitter::literal_to_instruction(ProtoType* l,
                                                        OI* context) {
  if(l->isA("ProtoScalar")) {
    float val = dynamic_cast<ProtoScalar*>(l)->value;
    if(val == (int)val) { // try to use integer
      if (val >= 0 && val < MAX_LIT_OPS) {
        return new Instruction(LIT_0_OP+(uint8_t)val);
      } else if (val >= 0 && val < 128) { 
        Instruction* i = new Instruction(LIT8_OP); i->padd((int)val); return i;
      } else if (val >= 0 && val < 65536) {
        Instruction* i = new Instruction(LIT16_OP);i->padd16((int)val);return i;
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
      chain_i(&deftup,literal_to_instruction(t->types[i],context));
    chain_i(&deftup,new iDEF_TUP(t->types.size(),true));
    chain_i(&end,chain_start(deftup)); // add to global program
    // make a global reference to it
    return new Reference(deftup,context);
  } else if(l->isA("ProtoLambda")) {
    // Branch lambdas are handled with a special case on the branch primitive:
    bool is_branch = true;
    for_set(Consumer,context->output->consumers,i)
      if(i->first->op!=Env::core_op("branch")) { is_branch=false; break; }
    if(is_branch) return new NoInstruction();
    // Otherwise, ensure the function is defined and get a reference
    // NOT YET IMPLEMENTED
  }
  // Catch-all fall-through case:
  ierror("Don't know how to emit literal: "+l->to_str());
}

/**
 * Helper function to create the proper REF_OP
 * (e.g., REF_0_OP, REF_OP 12, etc.)
 */
Instruction* newRefInstr(int index) {
   Instruction* ret = NULL;
   if(index < MAX_REF_OPS) ret = new Instruction(REF_0_OP + index);
   else if(index < 256) {ret = new Instruction(REF_OP); ret->padd(index);}
   else ierror("Environment reference too large: "+i2s(index));
   return ret;
}

Instruction* ProtoKernelEmitter::parameter_to_instruction(Parameter* p) {
   return newRefInstr(p->index);
}

// adds a tuple to the global declarations, then references it in vector op i
Instruction* ProtoKernelEmitter::vec_op_store(ProtoType* t) {
  ProtoTuple* tt = dynamic_cast<ProtoTuple*>(t);
  return chain_i(&end,new iDEF_TUP(tt->types.size()));
}

void mark_branch_references(Branch* jmp,Block* branch,AM* source) {
  Instruction* ptr = branch->contents;
  while(ptr) {
    // mark references that aren't already handled by an interior branch
    if(ptr->isA("Reference")) {
      OI* target = (OI*)((CEAttr*)ptr->attributes["~Ref~Target"])->value;
      if(target->output->domain == source) {
        if(ptr->attributes.count("~Branch~End"))
          ierror("Tried to duplicate mark reference");
        //cout << "  Marking reference for "<<ce2s(source)<<endl;
        //cout << "    "<<ce2s(jmp->after_this)<<endl;
        ptr->attributes["~Branch~End"]=new CEAttr(jmp->after_this);
      }
    } else if(ptr->isA("Block")) {
      mark_branch_references(jmp,(Block*)ptr,source);
    }
    ptr = ptr->next;
  }
}

// takes OperatorInstance because sometimes the operator type depends on it
Instruction* ProtoKernelEmitter::primitive_to_instruction(OperatorInstance* oi){
  Primitive* p = (Primitive*)oi->op;
  ProtoType* otype = oi->output->range; bool tuple = otype->isA("ProtoTuple"); 
  if(primitive2op.count(p->name)) { // plain ops
    if(tuple)return new Reference(primitive2op[p->name],vec_op_store(otype),oi);
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
        chain_i(&chain,new Reference(c,vec_op_store(otype),oi));
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
      chain_i(&chain,new Reference(VMUL_OP,vec_op_store(otype),oi));
      chain_i(&chain,new Instruction(POP_LET_2_OP,-2));
    }
    return chain_start(chain);
  } else if(p->name=="tup") {
    Instruction* i = new Reference(TUP_OP,vec_op_store(otype),oi); 
    i->stack_delta = 1-oi->inputs.size(); i->padd(oi->inputs.size());
    return i;
  } else if(p==Env::core_op("branch")) {
    Instruction *chain=NULL;
    // dump lambdas into branches
    Instruction *t_br, *f_br;
    t_br = dfg2instructions(((CompoundOp*)L_VAL(oi->inputs[1]->range))->body);
    f_br = dfg2instructions(((CompoundOp*)L_VAL(oi->inputs[2]->range))->body);
    // trim off DEF & RETURN
    t_br = t_br->next; f_br = f_br->next;
    chain_delete(chain_start(t_br),chain_start(t_br));
    chain_delete(chain_end(t_br),chain_end(t_br));
    chain_delete(chain_start(f_br),chain_start(f_br));
    chain_delete(chain_end(f_br),chain_end(f_br));
    t_br = new Block(t_br); f_br = new Block(f_br);
    // pull references for branch contents
    OIset handled_frags;
    for_map(OI*,CE*,fragments,i) {
      if(i->first->output->domain==oi->output->domain) {
        Instruction* ptr = chain_start((Instruction*)i->second);
        chain_i(&chain,ptr); handled_frags.insert(i->first);
      }
    }
    for_set(OI*,handled_frags,i) fragments.erase(*i);
    // string them together into a branch
    Branch* jmp = new Branch(t_br,true);
    mark_branch_references(jmp,(Block*)t_br,oi->output->domain);
    mark_branch_references(jmp,(Block*)f_br,oi->output->domain);
    chain_i(&chain,new Branch(jmp));
    chain_i(&chain,f_br);
    chain_i(&chain,jmp);
    chain_i(&chain,t_br);
    return chain_start(chain);
  } else if(p==Env::core_op("reference")) {
    return new NoInstruction(); // reference already created by input
  }
  // fall-through error case
  ierror("Don't know how to convert op to instruction: "+p->to_str());
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
      op_stackdeltas[i] = (int)((SE_Scalar*)(*op)[1])->value;
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
  verbosity = args->extract_switch("--emitter-verbosity") ? 
    args->pop_int() : parent->verbosity;
  max_loops=args->extract_switch("--emitter-max-loops")?args->pop_int():10;
  paranoid = args->extract_switch("--emitter-paranoid")|parent->paranoid;
  // load operation definitions
  string name = "core.ops"; load_ops(name,parent); terminate_on_error();
  // setup rule collection
  rules.push_back(new DeleteNulls(this,args));
  rules.push_back(new InsertLetPops(this,args));
  rules.push_back(new ResolveISizes(this,args));
  rules.push_back(new ResolveLocations(this,args));
  rules.push_back(new StackEnvSizer(this,args));
  rules.push_back(new ResolveState(this,args));
  // program starts empty
  start=end=NULL;
}

// A Field needs a let if it has:
// 1. More than one consumer in the same function
// 2. A consumer inside a different relevant function
bool needs_let(Field* f) {
  int consumer_count = 0;
  for_set(Consumer,f->consumers,c) {
    if(c->first->output->domain == f->domain) { // same function consumer
      consumer_count++;
      if(consumer_count > 1) return true; // 2+ => let needed
    } else if(f->container->relevant.count(c->first->output->domain)) {
      return true; // found a function reference
    }
  }
  return false;
}

/* this walks a DFG in order as follows: 
   - producer before consumers
   - consumers in order
*/
Instruction* ProtoKernelEmitter::tree2instructions(Field* f) {
  if(memory.count(f))
    { return new Reference((Instruction*)memory[f],f->producer); }
  OperatorInstance* oi = f->producer; Instruction* chain = NULL;
  // first, get all the inputs
  for(int i=0;i<oi->inputs.size();i++) 
    chain_i(&chain,tree2instructions(oi->inputs[i]));
  // second, add the operation
  if(oi->op==Env::core_op("reference")) {
    V4 << "    Reference is: " << ce2s(oi->op) << endl;
    if(oi->inputs.size()!=1) ierror("Bad number of reference inputs");
    Instruction* frag = chain_split(chain);
    if(frag) fragments[oi->inputs[0]->producer] = frag;
  } else if(oi->op->isA("Primitive")) {
    V4 << "    Primitive is: " << ce2s(oi->op) << endl;
    chain_i(&chain,primitive_to_instruction(oi));
  } else if(oi->op->isA("Literal")) { 
    V4 << "    Literal is: " << ce2s(oi->op) << endl;
    chain_i(&chain,literal_to_instruction(((Literal*)oi->op)->value,oi));
  } else if(oi->op->isA("Parameter")) { 
    V4 << "    Parameter is: " << ce2s(oi->op) << endl;
    chain_i(&chain,parameter_to_instruction((Parameter*)oi->op));
  } else if(oi->op->isA("CompoundOp")) { 
    V4 << "    Compound OP is: " << ce2s(oi->op) << endl;
    CompoundOp* cop = ((CompoundOp*)oi->op);
    Global* def_fun_instr = ((Global*)globalNameMap[cop]);
    Reference* glo_ref = new Reference(def_fun_instr,oi);
    chain_i(&chain,glo_ref); //needs offset to resolve
    Instruction* fun_call = new FunctionCall(cop);
    chain_i(&chain,fun_call);
  } else { // also CompoundOp, Parameter
    ierror("Don't know how to emit instruction for "+oi->op->to_str());
  }
  // finally, put the result in the appropriate location
  if(f->selectors.size())
    ierror("Restrictions not all compiled out for: "+f->to_str());
  if(needs_let(f)) { // need a let to contain this
    memory[f] = chain_i(&chain, new iLET());
    chain_i(&chain,new Reference((Instruction*)memory[f],oi)); // and we got here by a ref...
  }
  return chain_start(chain);
}

bool has_relevant_consumer(Field* f) {
  for_set(Consumer,f->consumers,c) {
    if(c->first->output->domain == f->domain) { // same function consumer
      return true;
    } else if(f->container->relevant.count(c->first->output->domain)) {
      return true; // found a function reference
    }
  }
  return false;
}

Instruction* ProtoKernelEmitter::dfg2instructions(AM* root) {
  Fset minima, f; root->all_fields(&f);
  for_set(Field*,f,i) if(!has_relevant_consumer(*i)) minima.insert(*i);
  
  V3 << "  "+ce2s(root)+":" << i2s(minima.size()) << " minima identified: ";
  for_set(Field*,minima,i) { if(!(i==minima.begin())) V3<<","; V3<<ce2s(*i); }
  V3 << endl;
  iDEF_FUN *fnstart = new iDEF_FUN(); Instruction *chain=fnstart;
  for_set(Field*,minima,i) chain_i(&chain, tree2instructions(*i));
  if(minima.size()>1) { // needs an all
    Instruction* all = new Instruction(ALL_OP);
    if(minima.size()>=256) ierror("Too many minima: "+minima.size());
    all->stack_delta = -(minima.size()-1); all->padd(minima.size());
    chain_i(&chain, all);
  }
  chain_i(&chain, fnstart->ret = new Instruction(RET_OP));
  if(root->bodyOf!=NULL) {
     V3 << "  Adding fn:" << root->bodyOf->name << " to globalNameMap" << endl;
     globalNameMap[root->bodyOf] = fnstart;
  }
  return fnstart;
}

string hexbyte(uint8_t v) {
  string hex = "0123456789ABCDEF";
  string out = "xx"; out[0]=hex[v>>4]; out[1]=hex[v & 0xf]; return out;
}

uint8_t* ProtoKernelEmitter::emit_from(DFG* g, int* len) {
  CheckEmittableType echecker(this); echecker.propagate(g);

  V1<<"Linearizing DFG to instructions...\n";
  start = end = new iDEF_VM(); // start of every script
  for_set(AM*,g->relevant,i) // translate each function
    if((*i)!=g->output->domain && !(*i)->marked("branch-fn"))
      chain_i(&end,dfg2instructions(*i));
  chain_i(&end,dfg2instructions(g->output->domain)); // next the main
  chain_i(&end,new Instruction(EXIT_OP)); // add the end op
  if(verbosity>=2) print_chain(start,cpout,2);

  // Check that we were able to linearize everything:
  if(fragments.size()) {
    ostringstream s; 
    print_chain(chain_start((Instruction*)fragments.begin()->second),&s,2);
    ierror("Unplaced fragment: "+ce2s(fragments.begin()->first)+"\n"+s.str());
  }

  V1<<"Resolving unknowns in instruction sequence...\n";
  for(int i=0;i<max_loops;i++) {
    bool changed=false;
    for(int j=0;j<rules.size();j++) {
      changed |= rules[j]->propagate(start); terminate_on_error();
    }
    if(!changed) break;
    if(i==(max_loops-1))
      compile_warn("Emitter analyzer giving up after "+i2s(max_loops)+" loops");
  }
  CheckResolution rchecker(this); rchecker.propagate(start);
  
  // finally, output
  V1<<"Outputting final instruction sequence...\n";
  *len= end->next_location();
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
