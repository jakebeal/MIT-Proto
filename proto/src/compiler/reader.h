/* Reader finds text and turns it into s-expressions
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __READER__
#define __READER__

#include <string>
#include <list>
#include <fstream>
#include "lisp.h"

using namespace std; // allow c-strings, etc; note: shadows 'pair'

typedef enum {
  Token_eof,
  Token_string,
  Token_symbol,
  Token_true,
  Token_false,
  Token_quote,
  Token_char,
  Token_left_paren,
  Token_right_paren,
} Token_type;

typedef struct {
  Token_type type;
  char *name;
} Token;

extern Obj *read_object (const char *string, int *start);

extern List *qq_env (const char *str, Obj *val, ...);
extern Obj *read_qq (const char *str, List *env);

extern Obj *read_from_str (const char *str);


// New-style path handling
struct Path {
  list<string> dirs;
  
  void add_default_path(string srcdir) {
    dirs.push_back(srcdir + ".");
    if (srcdir != "") {
      // use srcdir-relative paths
      dirs.push_back(srcdir + "/lib/");
      dirs.push_back(srcdir + "/lib/core/");
    } else {
      // use the install location
      dirs.push_back(PROTOLIBDIR);
    }
  }
  void add_to_path(string addition) { dirs.push_back(addition); }
  ifstream* find_in_path(char *filename){string s = filename; find_in_path(s);}
  ifstream* find_in_path(string filename);
};

extern List *read_objects_from (char *filename);
extern List *read_objects_from_dirs (string filename, Path *path);

#endif
