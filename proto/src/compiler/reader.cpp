/* Reader finds text and turns it into s-expressions
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "reader.h"

#include <iostream>

void Path::add_default_path(string srcdir) {
  dirs.push_back(srcdir + ".");
  if (srcdir != "") {
    // use srcdir-relative paths
    dirs.push_back(srcdir + "/lib/");
    dirs.push_back(srcdir + "/lib/core/");
  }
  // always use the install location
  dirs.push_back(PROTOLIBDIR);
}

ifstream* Path::find_in_path(string filename) {
  ifstream* s = new ifstream();
  for(list<string>::iterator i=dirs.begin(); i!=dirs.end(); i++) {
    string fullname = *i+"/"+filename;
    s->open(fullname.c_str());
    if(s->good()) return s;
    s->close();
  }
  delete s;
  return NULL;
}

void print_token (Token *token) {
  switch (token->type) {
  case Token_left_paren: post("LP"); break;
  case Token_right_paren: post("RP"); break;
  case Token_symbol: post("N(%s)", (char*)token->name); break;
  case Token_string: post("S(%s)", (char*)token->name); break;
  case Token_eof: post("EF"); break;
  }
}

Token *new_token (Token_type type, const char *name, int max_size) {
  Token *tok = (Token*)MALLOC(sizeof(Token));
  char  *buf = (char*)MALLOC(strlen(name)+1);
  if (strlen(name) > max_size)
    uerror("NEW TOKEN NAME OVERFLOW %d\n", max_size);
  strcpy(buf, name);
  tok->type = type;
  tok->name = buf;
  // print_token(tok); debug("\n");
  return tok;
}

#define BUF_SIZE 1024

#define OSTR_CHR '|'
#define  STR_CHR '\"'
#define RSTR_CHR '\''
#define Q_CHR    '\''
#define H_CHR    '#'
#define C_CHR    '\''

Token *read_token (const char *string, int *start) {
  int  is_str = 0;
  int  i = *start;
  int  j = 0;
  int  len = strlen(string);
  int  is_raw_str = 0;
  int  is_ostr = 0;
  int  is_hash = 0;
  char c;
  char buf[BUF_SIZE];
  // post("READING TOKEN %s %d\n", string, *start);
  for (;;) {
    if (j >= BUF_SIZE)
      uerror("BUF OVERFLOW\n");

    if (i < len) {
      c = string[i++]; *start = i;
      switch (c) {
      case ' ': case '\t': case '\n': case '\r': break;
      case '(': return new_token(Token_left_paren,  "(", 1);
      case ')': return new_token(Token_right_paren, ")", 1);
      case H_CHR:
        if (i < len) {
          c = string[i++];  *start = i;
          if (c == C_CHR)
            return new_token(Token_char, "#/", 2);
          else if (c == 'T' || c == 't')
            return new_token(Token_true, "#T", 2);
          else if (c == 'F' || c == 'f')
            return new_token(Token_false, "#F", 2);
          else
            uerror("BAD CHAR TOKEN %s %d\n", string, i);
        } else
          uerror("BAD CHAR TOKEN %s %d\n", string, i);
      case OSTR_CHR:
        is_ostr = 1;
        /* Fall through */
      case STR_CHR:
        is_str = 1; goto ready;
        // case RSTR_CHR: is_raw_str = 1; is_str = 1; goto ready;
      case Q_CHR: return new_token(Token_quote, "\'", 1);
      case ';':
        while (i < len) {
          c = string[i++]; *start = i;
          if (c == '\n' || c == '\r')
            break;
        }
        return read_token(string, start);
      default:  buf[j++] = c; buf[j] = 0; goto ready;
      }
    } else {
      return new_token(Token_eof, "", 1);
    }
  }
 ready:
  if (is_str) {
    int is_esc = 0;
    for (; i < len && j < BUF_SIZE; ) {
      c      = string[i++];
      *start = i;
      if (!is_raw_str && c == '\\') {
        is_esc = 1;
        continue;
      }
      if ((!is_esc && !is_raw_str &&
           ((!is_ostr && c == STR_CHR) || (is_ostr && c == OSTR_CHR))) ||
          (is_raw_str && c == RSTR_CHR)) {
        return new_token(Token_string, buf, BUF_SIZE);
      }
      buf[j++] = (is_esc && c == 'n') ? '\n' : c;
      buf[j]   = 0;
      is_esc   = 0;
    }
    uerror("unable to find end of string %s\n", buf);
  } else {
    for (; i < len; ) {
      c      = string[i++];
      *start = i;
      if (c == ')' || c == '('  || c == ' ' || c == '\t' || c == '\n' || c == '\r') {
        if (c == ')' || c == '(')
          *start -= 1;
        return new_token(Token_symbol, buf, BUF_SIZE);
      }
      buf[j++] = c;
      buf[j]   = 0;
    }
    *start = i;
    return new_token(Token_symbol, buf, BUF_SIZE);
  }
}

extern Obj *read_from (Token *token, const char *string, int *start);

List *read_list (char *string, int *start) {
  List *_list  = lisp_nil;
  // debug("READING LIST %d\n", *start);
  for (;;) {
    Token *token = read_token(string, start);
    Obj   *expr;
    // print_token(token); debug(" READ LIST TOKEN %d\n", *start);
    switch (token->type) {
      case Token_right_paren:
      case Token_eof:
	// debug("DONE READING LIST\n");
	return lst_rev(_list);
    }
    expr = read_from(token, string, start);
    // post("PAIRING "); print_object(list); post("\n");
    _list = new List(expr, _list);
  }
}

int isnum (char *name) {
  int i, nump;
  nump = isdigit(name[0]) || (name[0] == '-' && strlen(name) > 1);
  for (i = 1; i < strlen(name); i++) {
    nump = nump && (isdigit(name[i]) || name[i] == '.');
  }
  return nump;
}

Obj *new_sym_or_num(char *name) {
  if (isnum(name)) {
    int inum; flo fnum;
    int res = sscanf(name, "%f", &fnum);
    if (res == 1) {
      return new Number(fnum);
    } else {
      res = sscanf(name, "%d", &inum);
      if (res == 1) {
	return new Number(inum);
      } else
	uerror("UNABLE TO PARSE NUM %s", name);
    }
  } else
    return new Symbol(name);
}

Obj *read_from (Token *token, const char *string, int *start) {
  // post("READING FROM %s\n", &string[*start]);
  switch (token->type) {
  case Token_quote:
    return PAIR(new Symbol("QUOTE"), PAIR(read_object(string, start), lisp_nil));
  case Token_char: {
    Obj *obj = read_object(string, start);
    if (numberp(obj))
      return new Number('0' + ((Number*)obj)->getValue());
    else if (symbolp(obj))
      return new Number(((Symbol*)obj)->getName()[0]);
    else
      uerror("BAD CHAR TOKEN\n");
  }
  case Token_string:      return new String(token->name);
  case Token_true:        return new Number(1);
  case Token_false:       return new Number(0);
  case Token_symbol:      return new_sym_or_num(token->name);
  case Token_left_paren:  return read_list((char*)string, start);
  case Token_right_paren: uerror("Unbalanced parens\n");
  case Token_eof:         return NULL;
  default:                uerror("Unknown token type %d\n", token->type);
  }
}

Obj *read_object (const char *string, int *start) {
  Token *token = read_token(string, start);
  return read_from(token, string, start);
}

#define FILE_BUF_SIZE 100000

int copy_from_file (ifstream *file, char *buf) {
  int i=0;
  while(file->good() && i<FILE_BUF_SIZE-1) buf[i++] = file->get();
  if(i==FILE_BUF_SIZE-1) uerror("FILE READING BUFFER OVERFLOW %d\n", i);
  buf[i-1] = 0; // minus 1 because last character came from EOF
  return 1;
}

List *read_objects_from (ifstream *file) {
  int    start = 0;
  List   *objs = lisp_nil;
  char   buf[FILE_BUF_SIZE];

  if (!copy_from_file(file, buf))
    return NULL;
  for (;;) {
    Obj *obj = read_object(buf, &start);
    if (obj == NULL)
      return lst_rev(objs);
    else
      objs = new List(obj, objs);
  }
}

List *read_objects_from_dirs (string filename, Path *path) {
  ifstream* file = path->find_in_path(filename);
  if(file==NULL) { return NULL; }
  List *res = read_objects_from(file);
  delete file; return res;
}

List *qq_env (const char *str, Obj *val, ...) {
  int i, n;
  va_list ap;
  List *res = PAIR(PAIR(new Symbol(str), PAIR(val, lisp_nil)), lisp_nil);
  va_start(ap, val);
  for (n = 1; ; n++) {
    char *s = va_arg(ap, char *);
    if (s == NULL) break;
    Obj *v = va_arg(ap, Obj *);
    if (v == NULL) break;
    res = PAIR(PAIR(new Symbol(s), PAIR(v, lisp_nil)), res);
  }
  va_end(ap);
  return lst_rev(res);
}

Obj *read_from_str (const char *str) {
  int    j = 0;
  Obj *obj = read_object(str, &j);
  return obj;
}

Obj *qq_lookup(const char *name, List *env) {
  int i;
  List *args = lisp_nil;
  for (i = 0; i < lst_len(env); i++) {
    List *binding = (List*)lst_elt(env, i);
    if (sym_name(lst_elt(binding, 0)) == name)
      return lst_elt(binding, 1);
  }
  uerror("Unable to find qq_binding %s", name);
}

Obj *copy_eval_quasi_quote(Obj *obj, List *env) {
  if (obj->lispType() == LISP_SYMBOL) {
    if (sym_name(obj)[0] == '$') {
      return qq_lookup(sym_name(obj).c_str(), env);
    } else
      return obj;
  } else if (obj->lispType() == LISP_NUMBER) {
    return obj;
  } else if (obj->lispType() == LISP_LIST) {
    int i;
    int is_dot = 0;
    List *args = lisp_nil;
    for (i = 0; i < lst_len((List*)obj); i++) {
      Obj *copy = copy_eval_quasi_quote(lst_elt((List*)obj, i), env);
      if (is_dot) {
	args = lst_rev(args);
	List *a = args;
	while (a->getTail() != lisp_nil)
	  a = (List*)a->getTail();
	a->setTail(copy);
	return (Obj*)args;
      } else if (copy->lispType() == LISP_SYMBOL && sym_name(copy) == ".")
	is_dot = 1;
      else
	args = PAIR(copy, args);
    }
    return (Obj*)lst_rev(args);
  } else
    uerror("Unknown quasi quote element %s", obj->typeName());
}

Obj *read_qq (const char *str, List *env) {
  Obj *obj = read_from_str(str);
  return copy_eval_quasi_quote(obj, env);
}


