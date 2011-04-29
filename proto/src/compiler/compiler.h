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

struct ProtoInterpreter; class NeoCompiler;

/*****************************************************************************
 *  INTERPRETER                                                              *
 *****************************************************************************/

/**
 * Binding environments: tracks name/object associations during interpretation
 */
struct Env {
  Env* parent; ProtoInterpreter* cp;
  map<string,CompilationElement*> bindings;
  
  Env(ProtoInterpreter* cp) { parent=NULL; this->cp = cp; }
  Env(Env* parent) { this->parent=parent; cp = parent->cp; }
  void bind(string name, CompilationElement* value);
  void force_bind(string name, CompilationElement* value);

  /**
   * Lookups: w/o type, returns NULL on failure; w. type, checks, returns dummy
   */
  CompilationElement* lookup(string name, bool recursed=false);
  CompilationElement* lookup(SE_Symbol* sym, string type);
  
  /**
   * Operators needed to be accessed unshadowed by compiler. These are gathered
   * after initialization, but before user code is loaded.
   */
  static map<string,Operator*> core_ops;
  static void record_core_ops(Env* toplevel);
  static Operator* core_op(string name);
};

class ProtoInterpreter {
 public:
  friend class Env;
  Env* toplevel;
  AM* allspace;
  DFG* dfg;
  NeoCompiler* parent;
  int verbosity;

  ProtoInterpreter(NeoCompiler* parent, Args* args);
  ~ProtoInterpreter();

  void interpret(SExpr* sexpr);
  static bool sexp_is_type(SExpr* s);
  static ProtoType* sexp_to_type(SExpr* s);
  static Signature* sexp_to_sig(SExpr* s,Env* bindloc=NULL,CompoundOp* op=NULL,AM* space=NULL);
  static pair<string,ProtoType*> parse_argument(SE_List_iter* i, int n, Signature* sig, bool anonymous_ok=true);
  
 private:
  /**
   * FOR INTERNAL USE ONLY
   */
  void interpret(SExpr* sexpr, bool recursed);
  void interpret_file(string name);
  
  Operator* sexp_to_op(SExpr* s, Env *env);  
  Macro* sexp_to_macro(SE_List* s, Env *env);  
  Signature* sexp_to_macro_sig(SExpr* s);
  Field* sexp_to_graph(SExpr* s, AM* space, Env *env);
  SExpr* expand_macro(MacroOperator* m, SE_List* call);
  SExpr* macro_substitute(SExpr* src,Env* e,SE_List* wrapper=NULL);
  ProtoType* symbolic_literal(string name);

  /// compiler special-form handlers
  Field* let_to_graph(SE_List* s, AM* space, Env *env,bool incremental);
  /// compiler special-form handlers
  Field* letfed_to_graph(SE_List* s, AM* space, Env *env,bool no_init);
  /// compiler special-form handlers
  Field* restrict_to_graph(SE_List* s, AM* space, Env *env);
};

/**
 * RULE-BASED GRAPH TRANSFORMATION
 */
class DFGTransformer {
 public:
  vector<IRPropagator*> rules;
  bool paranoid;
  int max_loops, verbosity;

  DFGTransformer() {}
  ~DFGTransformer() {}
  
  virtual void transform(DFG* g);
};

class ProtoAnalyzer : public DFGTransformer {
 public:
  ProtoAnalyzer(NeoCompiler* parent, Args* args);
  ~ProtoAnalyzer() {}
};

class GlobalToLocal : public DFGTransformer {
 public:
  GlobalToLocal(NeoCompiler* parent, Args* args);
  ~GlobalToLocal() {}
};

/**
 * CODE EMITTER.
 *  this is a framework class: we'll have multiple instantiations
 */
class CodeEmitter {
  public:
    /**
     * If an emitter wants to change some of the core operators, it must
     * call Env::record_core_ops(parent->interpreter->toplevel)
     * after it has loaded its modified definitions
     */
    virtual uint8_t* emit_from(DFG* g, int* len) = 0;
};

class InstructionPropagator; class Instruction;

class ProtoKernelEmitter : public CodeEmitter {
 public:
  bool is_dump_hex, paranoid; 
  static bool op_debug;
  int max_loops, verbosity, print_compact;
  NeoCompiler* parent;

  ProtoKernelEmitter(NeoCompiler* parent, Args* args);
  uint8_t* emit_from(DFG* g, int* len);

  /// map of compound ops -> instructions (in global mem)
  map<CompoundOp*,Instruction*> globalNameMap;

 private:
  vector<InstructionPropagator*> rules;

  /// global & env storage
  map<Field*,CompilationElement*, CompilationElement_cmp> memory;
  /// fragments floating up to find a home
  map<OI*,CompilationElement*, CompilationElement_cmp> fragments;

  /// list of scalar/vector ops
  map<string,pair<int,int> > sv_ops; 
  Instruction *start, *end;

  void load_ops(string name, NeoCompiler* parent);
  Instruction* tree2instructions(Field* f);
  Instruction* primitive_to_instruction(OperatorInstance* oi);
  Instruction* literal_to_instruction(ProtoType* l, OperatorInstance* context);
  Instruction* parameter_to_instruction(Parameter* param);
  Instruction* dfg2instructions(AM* g);

  /// allocates globals for vector ops
  Instruction* vec_op_store(ProtoType* t);
};

/*****************************************************************************
 *  TOP-LEVEL COMPILER                                                       *
 *****************************************************************************/

/**
 * Interface class (shared w. PaleoCompiler)
 */
class Compiler : public EventConsumer {
 public:
  Compiler(Args* args) {}
  ~Compiler() {}
  // setup output files as standalone app
  virtual void init_standalone(Args* args) = 0;
  // compile expression str; len is filled in w. output length
  virtual uint8_t* compile(const char *str, int* len) = 0;
  virtual void set_platform(string path) = 0;
};

/**  
 * NeoCompiler implementation
 */
class NeoCompiler : public Compiler {
 public:
  Path proto_path;
  bool is_dump_code, is_dump_all, is_dump_analyzed, is_dump_interpreted,
    is_dump_raw_localized, is_dump_localized, is_dump_dotfiles,is_dotfields; 
  string dotstem;
  int is_early_terminate;
  bool paranoid; int verbosity;
  string infile;
  ProtoInterpreter* interpreter;
  DFGTransformer *analyzer, *localizer;
  CodeEmitter* emitter;
  /// the last piece of text fed to start the compiler
  const char* last_script;

 public:
  NeoCompiler(Args* args);
  ~NeoCompiler();
  void init_standalone(Args* args);

  uint8_t* compile(const char *str, int* len);
  void set_platform(string path);
};

/// list of internal tests:
void type_system_tests();

#endif // __NEOCOMPILER__
