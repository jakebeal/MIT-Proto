/* Proto compiler
Copyright (C) 2009, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// During experimental development, the NeoCompiler will be worked on
// "off to the side" of the original, not replacing it.

#ifndef __NEOCOMPILER__
#define __NEOCOMPILER__

#include "config.h"
#include <stdint.h>
#include "utils.h"
#include "ir.h"
#include "analyzer.h"
#include "reader.h" // for paths

struct ProtoInterpreter; struct NeoCompiler;

/*****************************************************************************
 *  INTERPRETER                                                              *
 *****************************************************************************/

// Binding environments (used by interpreter only)
struct Env {
  Env* parent; ProtoInterpreter* cp;
  map<string,CompilationElement*> bindings;
  
  Env(ProtoInterpreter* cp) { parent=NULL; this->cp = cp; }
  Env(Env* parent) { this->parent=parent; cp = parent->cp; }
  void bind(string name, CompilationElement* value);
  void force_bind(string name, CompilationElement* value);
  // Lookups: w/o type, returns NULL on failure; w. type, checks, returns dummy
  CompilationElement* lookup(string name, bool recursed=false);
  CompilationElement* lookup(SE_Symbol* sym, string type);
  
  // operators needed to be accessed unshadows by compiler
  static Operator *RESTRICT, *DELAY, *DCHANGE, *NOT, *MUX, *TUP, *ELT, *LOCAL;
};

class ProtoInterpreter {
 public:
  friend class Env;
  Env* toplevel;
  AM* allspace;
  DFG* main;
  NeoCompiler* parent;

  bool is_dump_interpretation;

  ProtoInterpreter(NeoCompiler* parent, Args* args);
  ~ProtoInterpreter();

  void interpret(SExpr* sexpr);

 private:
  void interpret(SExpr* sexpr, bool recursed); // internal only
  void interpret_file(string name);
  
  Operator* sexp_to_op(SExpr* s, Env *env);  
  Macro* sexp_to_macro(SE_List* s, Env *env);  
  ProtoType* sexp_to_type(SExpr* s);
  Signature* sexp_to_sig(SExpr* s,Env* bindloc=NULL,CompoundOp* op=NULL,AM* space=NULL);
  Signature* sexp_to_macro_sig(SExpr* s);
  Field* sexp_to_graph(SExpr* s, DFG* g, AM* space, Env *env);
  SExpr* expand_macro(MacroOperator* m, SE_List* call);
  SExpr* macro_substitute(SExpr* src,Env* e,SE_List* wrapper=NULL);
  Operator* symbolic_literal(string name);

  // compiler special-form handlers
  Field* let_to_graph(SE_List* s, DFG* g, AM* space, Env *env,bool incremental);
  Field* letfed_to_graph(SE_List* s, DFG* g, AM* space, Env *env);
  Field* restrict_to_graph(SE_List* s, DFG* g, AM* space, Env *env);
};

/*****************************************************************************
 *  OPTIMIZER                                                                *
 *****************************************************************************/

class ProtoAnalyzer {
 public:
  vector<Propagator*> rules;
  NeoCompiler* parent;

  bool is_dump_analyzed, paranoid;
  int max_loops, verbosity;

  ProtoAnalyzer(NeoCompiler* parent, Args* args);
  ~ProtoAnalyzer() {}
  
  void analyze(DFG* g);
};

/*****************************************************************************
 *  GLOBAL-TO-LOCAL TRANSFORMER                                              *
 *****************************************************************************/

class GlobalToLocal {
 public:
  vector<Propagator*> rules;
  NeoCompiler* parent;

  bool is_dump_localized, paranoid;
  int max_loops, verbosity;

  GlobalToLocal(NeoCompiler* parent, Args* args);
  ~GlobalToLocal() {}
  
  void localize(DFG* g);
};


/*****************************************************************************
 *  CODE EMITTER                                                             *
 *****************************************************************************/

// this is a framework class: we'll have multiple instantiations
class CodeEmitter {
 public:
  virtual uint8_t* emit_from(DFG* g, int* len) = 0;
};

class InstructionPropagator; class Instruction;
class ProtoKernelEmitter : public CodeEmitter {
 public:
  bool is_dump_hex, paranoid;
  int max_loops, verbosity, print_compact;
  NeoCompiler* parent;

  ProtoKernelEmitter(NeoCompiler* parent, Args* args);
  uint8_t* emit_from(DFG* g, int* len);

 private:
  vector<InstructionPropagator*> rules;
  map<Field*,CompilationElement*> memory; // global & env storage
  map<string,pair<int,int> > sv_ops; // list of scalar/vector ops
  Instruction *start, *end;

  void load_ops(string name, NeoCompiler* parent);
  Instruction* tree2instructions(Field* f);
  Instruction* primitive_to_instruction(OperatorInstance* oi);
  Instruction* literal_to_instruction(ProtoType* l);
  Instruction* dfg2instructions(DFG* g);
  Instruction* vec_op_store(ProtoType* t); // allocates globals for vector ops
};

/*****************************************************************************
 *  TOP-LEVEL COMPILER                                                       *
 *****************************************************************************/

// Interface class
class NeoCompiler : public EventConsumer {
 public:
  Path proto_path;
  bool is_dump_code, is_dump_all; int is_early_terminate;
  ProtoInterpreter* interpreter;
  ProtoAnalyzer* analyzer;
  GlobalToLocal* localizer;
  CodeEmitter* emitter;
  char* last_script; // the last piece of text fed to start the compiler

 public:
  NeoCompiler(Args* args);
  ~NeoCompiler();
  void init_standalone(Args* args);

  uint8_t* compile(char *str, int* len); // len is filled in w. output length
  void set_platform(string path);
};

// list of internal tests:
void type_system_tests();

#endif // __NEOCOMPILER__
