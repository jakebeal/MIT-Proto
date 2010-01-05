/* S-expressions
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __SEXPR__
#define __SEXPR__

#include <vector>
#include <stack>
//#include <iostream>
#include "compiler-utils.h"

using namespace std;

struct SExpr : CompilationElement {
  virtual string type_of() { return "SExpr"; }
  bool isSymbol() { return type_of()=="SE_Symbol"; }
  bool isList() { return type_of()=="SE_List"; }
  bool isScalar() { return type_of()=="SE_Scalar"; }

  virtual bool operator== (string s) { return false; }
  virtual bool operator== (float v) { return false; }
  virtual bool operator== (SExpr *ex) = 0;
  bool operator== (const char* s) { return (*this)==string(s); }
  
};

struct SE_Scalar: SExpr {
  float value;
  SE_Scalar(float value) { this->value = value; }
  void print(ostream *out=cpout) { *out << value; }
  string type_of() { return "SE_Scalar"; }

  virtual bool operator== (float v) { return v==value; }
  virtual bool operator== (SExpr *ex) {
    return ex->isScalar() && ((SE_Scalar*)ex)->value==value;
  }
};

struct SE_Symbol : SExpr {
  string name;
  SE_Symbol(string name) { 
    this->name = name;
    for(int i=0; i<this->name.size(); i++) // upcase the symbol
      this->name[i]=tolower(this->name[i]);
  }
  void print(ostream *out=cpout) { *out << name; }
  string type_of() { return "SE_Symbol"; }

  virtual bool operator== (string s) { return s==name; }
  virtual bool operator== (SExpr *ex) {
    return ex->isSymbol() && ((SE_Symbol*)ex)->name==name;
  }
};

struct SE_List : SExpr {
  vector<SExpr*> children;
  void add(SExpr* e) { children.push_back(e); inherit_attributes(e); }
  int len() { return children.size(); }
  SExpr* op() { return children[0]; }
  vector<SExpr*>::iterator args() { return children.begin()++; }
  void print(ostream *out=cpout) { 
    *out << "("; pp_push(1);
    for(vector<SExpr*>::iterator i=children.begin(); i!=children.end(); i++) {
      if(i!=children.begin()) { *out << endl << pp_indent(); }
      (*i)->print(out);
    }
    pp_pop(); *out << ")";
  }
  string type_of() { return "SE_List"; }

  virtual bool operator== (SExpr *ex) {
    if(!ex->isList()) return false;
    SE_List *l = (SE_List*)ex;
    if(l->len() != len()) return false;
    for(int i=0;i<len();i++) if(!(children[i]==l->children[i])) return false;
    return true;
  }
};

/****** Lexical analyzer for reading SExprs ******/

extern SExpr* read_sexpr(string name, string in);
extern SExpr* read_sexpr(string name, istream* in=0, ostream* out=0);

#endif // __SEXPR__
