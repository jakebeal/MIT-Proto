/* Standalone compiler app
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "compiler.h"

void run_test_suite(); // testing kludge

int main (int argc, char *argv[]) {
  post("PROTO v%d (%d OPS) (Developed by MIT Space-Time Programming Group 2005-2008)\n", PROTO_VERSION, CORE_CMD_OPS);
  Args *args = new Args(argc,argv); // set up the arg parser

  if(args->extract_switch("--test")) { run_test_suite(); exit(0); }

#if __USE_NEOCOMPILER__
  NeoCompiler* neocompiler = new NeoCompiler(args);
  neocompiler->init_standalone(args);
#else
  Compiler* compiler = new Compiler(args);
  compiler->init_standalone(args);
#endif

  // load the script
  int len;
  if(args->argc==1) {
#if __USE_NEOCOMPILER__
    post("WARNING: NOTHING TO COMPILE");
#else
    uint8_t* s = compiler->compile("(app)",&len);
#endif
  } else if(args->argc==2) {
#if __USE_NEOCOMPILER__
    uint8_t* s = neocompiler->compile(args->argv[args->argc-1],&len);
#else
    uint8_t* s = compiler->compile(args->argv[args->argc-1],&len);
#endif
  } else {
    post("WARNING: %d unhandled arguments:",args->argc-2);
    for(int i=2;i<args->argc;i++) post(" '%s'",args->argv[i-1]);
    post("\n");
  }

  exit(0);
}


/// test suite!
extern void test_compiler_utils();

void run_test_suite() {
  test_compiler_utils();
}
