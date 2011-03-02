/* Proto compiler utilities
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include <stack>
#include <list>
#include <stdlib.h>
#include "compiler-utils.h"
#include "utils.h"

#include "sexpr.h" // testing include

bool SExpr::isKeyword() { return (isSymbol() && ((SE_Symbol*)this)->name[0]==':'); }

bool SExpr::NO_LINE_BREAKS=true;

ostream *cpout=&cout, *cperr=&cerr, *cplog=&clog; // Compiler output streams

stack<int> pp_stack;
int pp_depth() { return (pp_stack.empty()) ? 0 : pp_stack.top(); }
void pp_push(int n) { pp_stack.push(n+pp_depth()); }
void pp_pop() { pp_stack.pop(); }
string pp_indent()
{ return string(pp_depth(),' '); }

void ierror(string msg) {
  *cperr << "COMPILER INTERNAL ERROR: " << msg << endl;
  exit(1);
}

void ierror(CompilationElement *where, string msg) {
  *cperr << "COMPILER INTERNAL ERROR ("; 
  where->attributes["CONTEXT"]->print(cperr);
  *cperr << "): " << msg << endl; 
  exit(1);
}

string b2s(bool b) { return b ? "true" : "false"; }
// f2s and i2s are wrappers that change a char* into a string
string f2s(float num, int precision) { return flo2str(num,precision); }
string i2s(int num) { return int2str(num); }

// graceful error reporting
string compile_phase = "initialization";
bool compiler_error = false;
bool compiler_test_mode = false;
void compile_error(CompilationElement *where,string msg) {
  if(!compiler_error)
    { compiler_error=true; *cperr << "Error during "+compile_phase+":\n"; }
  if(where->attributes["CONTEXT"]) where->attributes["CONTEXT"]->print(cperr);
  else *cperr << "[SOURCE UNKNOWN]";
  *cperr << " Error: " << msg << endl;
}
void compile_error(string msg) {
  if(!compiler_error)
    { compiler_error=true; *cperr << "Error during "+compile_phase+":\n"; }
  *cperr << " Error: " << msg << endl;
}
void compile_warn(CompilationElement *where,string msg) {
  if(!where->attributes["CONTEXT"]) 
    ierror("Context absent while trying to report warning '"+msg+"'");
  where->attributes["CONTEXT"]->print(cperr);
  *cperr << " Warning: " << msg << endl;
}
void compile_warn(string msg) { *cperr << " Warning: " << msg << endl; }

void terminate_on_error() {
  if(compiler_error) { 
    *cperr << "Compilation failed." << endl; 
    exit(!compiler_test_mode);
  }
}

uint32_t CompilationElement::max_id=0;

// STANDALONE TESTER: to run this test, modify the compiler to call it
void test_compiler_utils() {
  CompilationElement foo, bar, baz;
  foo.attributes["CONTEXT"] = new Context("sample",2);
  baz=foo;
  foo.attributes["CONTEXT"]->merge(new Context("sample",5));
  foo.attributes["CONTEXT"]->merge(new Context("simple",8));
  foo.attributes["CONTEXT"]->merge(new Context("wimple",3));
  foo.attributes["CONTEXT"]->merge(new Context("simple",7));
  foo.attributes["RANDOM"] = new Context("rnd",0);
  *cpout << "foo: "; foo.print(); // should have 3 CONTEXT, 1 RANDOM
  *cpout << "bar: "; bar.print(); // should have nothing
  *cpout << "baz: "; baz.print(); // should have same as foo

  SExpr* out = read_sexpr("cmdline","(1 (2 3) 4 ((5) 6))");
  if(out) *cpout << out->to_str() << endl; else *cpout << "Parse failed!\n";
  out = read_sexpr("cmdline","`(this ,@(is ,really) 'splicy)");
  if(out) *cpout << out->to_str() << endl; else *cpout << "Parse failed!\n";
  out = read_sexpr("cmdline",";; ignore me \nhere be  ;;comment\n symbols\n");
  if(out) *cpout << out->to_str() << endl; else *cpout << "Parse failed!\n";
  out = read_sexpr("cmdline","(busted wrap ,)");
  if(out) *cpout << out->to_str() << endl; else *cpout << "Parse failed!\n";
  out = read_sexpr("cmdline","~");
  if(out) *cpout << out->to_str() << endl; else *cpout << "Parse failed!\n";
  out = read_sexpr("cmdline","\a");
  if(out) *cpout << out->to_str() << endl; else *cpout << "Parse failed!\n";

  out = read_sexpr("cmdline","this|is|bar|separated");
  if(out) *cpout << out->to_str() << endl; else *cpout << "Parse failed!\n";

  *cpout << flush;
  exit(0);
  //ierror(&foo,"Behold the fail!");
}
