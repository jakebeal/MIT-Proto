/* Proto compiler
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __COMPILER__
#define __COMPILER__

#include <inttypes.h>
#include <string>

#define __USE_NEOCOMPILER__ 0

using namespace std;

extern void init_compiler(void);
extern uint8_t* compile_script (char *str, int *len, int is_dump_ast);
extern void dump_instructions(int is_c, int n, uint8_t *bytes);

#include "utils.h"
#include "proto_opcodes.h"

class Compiler : public EventConsumer {
 public:
  BOOL is_show_code;
  BOOL is_dump_code;
  BOOL is_dump_ast;
  const char* last_script;
  
  Compiler(Args* args);
  ~Compiler();
  void init_standalone(Args* args); // setup output files as standalone app
  uint8_t* compile(const char *str, int* len); // len is filled in w. output length
  void visualize();
  BOOL handle_key(KeyEvent* key);
  void set_platform(string path);
  void setDefops(string defops);
};

#endif //__COMPILER__
