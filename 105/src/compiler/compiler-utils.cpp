/* Proto compiler utilities
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include "compiler-utils.h"
#include <stack>
#include <list>
#include <stdlib.h>

#include "sexpr.h" // testing include
list<string>* read_enum(string in);

ostream *cpout=&cout, *cperr=&cerr, *cplog=&clog; // Compiler output streams

void ierror(string msg) {
  *cperr << "COMPILER INTERNAL ERROR:" << msg << endl;
  exit(1);
}

void ierror(CompilationElement *where, string msg) {
  *cperr << "COMPILER INTERNAL ERROR ("; 
  where->attributes["CONTEXT"]->print(cperr);
  *cperr << "): " << msg << endl; 
  exit(1);
}

stack<int> pp_stack;
int pp_depth() { return (pp_stack.empty()) ? 0 : pp_stack.top(); }
void pp_push(int n) { pp_stack.push(n+pp_depth()); }
void pp_pop() { pp_stack.pop(); }
string pp_indent()
{ return string(pp_depth(),' '); }

void test_compiler_utils() {
  CompilationElement foo, bar, baz;
  foo.attributes["CONTEXT"] = new Context("sample",2);
  baz=foo;
  foo.attributes["CONTEXT"]->merge(new Context("sample",5));
  foo.attributes["CONTEXT"]->merge(new Context("simple",8));
  foo.attributes["CONTEXT"]->merge(new Context("wimple",3));
  foo.attributes["CONTEXT"]->merge(new Context("simple",7));
  foo.attributes["RANDOM"] = new Context("rnd",0);
  *cpout << "foo: "; foo.print();
  *cpout << "bar: "; bar.print();
  *cpout << "baz: "; baz.print();

  SExpr* out = read_sexpr("cmdline","(1 (2 3) 4 ((5) 6))");
  if(out) *cpout << out->to_str() << endl; else *cpout << "Parse failed!\n";
  out = read_sexpr("cmdline","`(this ,@(is ,really) 'splicy)");
  if(out) *cpout << out->to_str() << endl; else *cpout << "Parse failed!\n";
  out = read_sexpr("cmdline",";; ignore me \nhere be  ;;comment\n symbols\n");
  if(out) *cpout << out->to_str() << endl; else *cpout << "Parse failed!\n";
  out = read_sexpr("cmdline","(busted wrap ,)");
  if(out) *cpout << out->to_str() << endl; else *cpout << "Parse failed!\n";
  out = read_sexpr("cmdline","|");
  if(out) *cpout << out->to_str() << endl; else *cpout << "Parse failed!\n";
  out = read_sexpr("cmdline","\a");
  if(out) *cpout << out->to_str() << endl; else *cpout << "Parse failed!\n";

  list<string>* strs = read_enum("typedef enum { /* comment */ DIE_OP = CORE_CMD_OPS, MAX_CMD_OPS } PLATFORM_OPCODES;");
  if(strs) *cpout << "Found " << strs->size() << endl; else *cpout << "Enum parse failed!\n";
  strs = read_enum("typedef enum { /* comment */ DIE_OP = CORE_CMD_OPS, } PLATFORM_OPNDES;");
  if(strs) *cpout << "Found " << strs->size() << endl; else *cpout << "Enum parse failed!\n";

  ierror(&foo,"Behold the fail!");
}
