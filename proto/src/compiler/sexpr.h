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

struct SExpr : CompilationElement { reflection_sub(SExpr,CE);
  static bool NO_LINE_BREAKS;
  bool isSymbol() { return type_of()=="SE_Symbol"; }
  bool isList() { return type_of()=="SE_List"; }
  bool isScalar() { return type_of()=="SE_Scalar"; }
  bool isKeyword();

  virtual bool operator== (string s) { return false; }
  virtual bool operator== (float v) { return false; }
  virtual bool operator== (SExpr *ex) = 0;
  bool operator== (const char* s) { return (*this)==string(s); }
  
  virtual SExpr* copy() = 0;
};

struct SE_Scalar: SExpr { reflection_sub(SE_Scalar,SExpr);
  float value;
  SE_Scalar(float value) { this->value = value; }
  SE_Scalar(SE_Scalar* src) { this->value=src->value; inherit_attributes(src); }
  SExpr* copy() { return new SE_Scalar(this); }
  void print(ostream *out=cpout) { *out << value; }

  virtual bool operator== (float v) { return v==value; }
  virtual bool operator== (SExpr *ex) {
    return ex->isScalar() && ((SE_Scalar*)ex)->value==value;
  }
};

struct SE_Symbol : SExpr { reflection_sub(SE_Symbol,SExpr);
  string name;
  static bool case_insensitive; // Neocompiler is case sensitive, Paleo is not
  SE_Symbol(string name) { 
    this->name = name;
    if(case_insensitive) {
      for(int i=0; i<this->name.size(); i++) // downcase the symbol
        this->name[i]=tolower(this->name[i]);
    }
  }
  SE_Symbol(SE_Symbol* src) { this->name=src->name; inherit_attributes(src);}
  SExpr* copy() { return new SE_Symbol(this); }
  void print(ostream *out=cpout) { *out << name; }

  virtual bool operator== (string s) { return s==name; }
  virtual bool operator== (SExpr *ex) {
    return ex->isSymbol() && ((SE_Symbol*)ex)->name==name;
  }
};

struct SE_List : SExpr { reflection_sub(SE_List,SExpr);
  vector<SExpr*> children;
  SExpr* copy() {
    SE_List *dst = new SE_List();
    dst->inherit_attributes(this);
    for(int i=0;i<len();i++) dst->add((*this)[i]->copy());
    return dst;
  }
  void add(SExpr* e) { children.push_back(e); inherit_attributes(e); }
  int len() { return children.size(); }
  SExpr* op() { return children[0]; }
  SExpr* operator[](int i) { return children[i]; }
  vector<SExpr*>::iterator args() { return ++children.begin(); }
  void print(ostream *out=cpout) { 
    *out << "("; pp_push(1);
    for(vector<SExpr*>::iterator i=children.begin(); i!=children.end(); i++) {
      if(i!=children.begin()) 
        { if(NO_LINE_BREAKS) *out << " "; else *out<< endl<< pp_indent(); }
      (*i)->print(out);
    }
    pp_pop(); *out << ")";
  }

  virtual bool operator== (SExpr *ex) {
    if(!ex->isList()) return false;
    SE_List *l = (SE_List*)ex;
    if(l->len() != len()) return false;
    for(int i=0;i<len();i++) if(!(children[i]==l->children[i])) return false;
    return true;
  }
};


/****** Attribute for carrying SExprs around ******/

struct SExprAttribute : Attribute { reflection_sub(SExprAttribute,Attribute);
  bool inherit; SExpr* exp;
  SExprAttribute(SExpr* s,bool inherit=true) { exp=s; this->inherit=inherit; }
  
  void print(ostream *out=cpout) { *out << "S: "; exp->print(out); }
  virtual Attribute* inherited() { return (inherit ? this : NULL); }
  // no merge defined
};

/****** Lexical analyzer for reading SExprs ******/

extern SExpr* read_sexpr(string name, string in);
extern SExpr* read_sexpr(string name, istream* in=0, ostream* out=0);


/****** Utility to make parsing SEList structures easier ******/
SExpr* sexp_err(CompilationElement *where,string msg); // error & return dummy

struct SE_List_iter {
  SE_List* container; int index;
  SE_List_iter(SE_List* s) { index=0; container=s; }
  SE_List_iter(SExpr* s,string name="expression") { // named used for error
    if(!s->isList()) {
      compile_error(s,"Expected "+name+" to be a list: "+ce2s(s));
      SExpr* news=new SE_List(); news->inherit_attributes(s); s=news;
    }
    index=0; container=(SE_List*)s;
  }
  // true if there are more elements
  bool has_next() { return (index < container->children.size()); }
  // if next element is SE_Symbol named s, return true and advances
  bool on_token(string s) {
    if(!has_next()) return false;
    if(!container->children[index]->isSymbol()) return false;
    if(!(((SE_Symbol*)container->children[index])->name==s)) return false;
    index++; return true;
  }
  // return next expression (if it exists); error if it does not
  SExpr* peek_next(string name="expression") {
    if(!has_next()) {return sexp_err(container,"Expected "+name+" is missing");}
    return container->children[index];
  }
  // return next expression (if it exists) and increment; error if it does not
  SExpr* get_next(string name="expression") {
    if(!has_next()) {return sexp_err(container,"Expected "+name+" is missing");}
    return container->children[index++];
  }
  // getters specialized for strings and numbers
  string get_token(string name="expression") {
    SExpr* tok = get_next(name);
    if(tok->isSymbol()) { return ((SE_Symbol*)tok)->name;
    } else { 
      compile_error(tok,"Expected "+name+" is not a symbol");
      return "ERROR";
    }
  }
  float get_num(string name="expression") {
    SExpr* tok = get_next(name);
    if(tok->isScalar()) { return ((SE_Scalar*)tok)->value;
    } else { 
      compile_error(tok,"Expected "+name+" is not a number");
      return -1;
    }
  }
  // backs up one expression; attempting to unread 0 gets 0
  void unread(int n=1) { index-=n; if(index<0) index=0; }
};



#endif // __SEXPR__
