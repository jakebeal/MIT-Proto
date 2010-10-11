/* Proto compiler utilities
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// Note: compiler will leak memory like mad, so it needs to be run 
//       as a sub-application, then discarded.

#ifndef __COMPILER_UTILS__
#define __COMPILER_UTILS__

#include <map>
#include <iostream>
#include <utility>
#include <string>
#include <sstream>
#include <vector>
#include "utils.h"

using namespace std;

extern ostream *cpout, *cperr, *cplog; // Compiler output streams

/****** OUTPUT HANDLING ******/
struct CompilationElement;

// Report a compiler internal error (i.e. not caused by user)
void ierror(string msg);
void ierror(CompilationElement *where, string msg);
// pretty-print indenting
void pp_push(int n=1);
void pp_pop();
string pp_indent();

string b2s(bool b);
string f2s(float num, int precision=2);
string i2s(int num);
string V2S(vector<CompilationElement*> *v);
#define v2s(x) (V2S((vector<CompilationElement*>*)(x)))

// ERROR REPORTING
// Using globals is a little bit ugly, but we're not planning to have
// the compiler be re-entrant any time soon, so it's fairly safe to assume
// we'll always have precisely one compiler running.
extern string compile_phase; // name of current phase, for error reporting
extern bool compiler_error; // when true, terminate & exit gracefully
extern bool compiler_test_mode; // when true, errors exit w. status 0
void compile_error(string msg);
void compile_error(CompilationElement *where,string msg);
void compile_warn(string msg);
void compile_warn(CompilationElement *where,string msg);
void terminate_on_error(); // clean-up & kill application

/****** COMPILATION ELEMENTS & ATTRIBUTES ******/
struct Attribute {
  virtual Attribute* inherited() { return NULL; }
  virtual void merge(Attribute *addition) { 
    ierror("Attempt to merge incompatible attributes: " + this->to_str()
	   + ", " + addition->to_str());
  };
  virtual bool isA(string c){ return (c=="Attribute")?true:false; }
  string to_str() { ostringstream s; print(&s); return s.str(); }
  virtual void print(ostream *out=cpout) = 0;
};


struct Context : Attribute {
  map<string, pair<int,int>*> places; // file, <start,end>
  
  Context(string file_name,int line)
    { places[file_name] = new pair<int,int>(line,line); }
  
  Context(Context& src) { // deep copy constructor
    map<string,pair<int,int>*>::iterator i;
    for(i=src.places.begin(); i != src.places.end(); i++) {
      places[i->first] = new pair<int,int>(i->second->first,i->second->second);
    }
  }
  Attribute* inherited() { return new Context(*this); }
  void merge(Attribute *_addition) {
    Context *addition = (Context*)_addition;
    map<string,pair<int,int>*>::iterator i;
    for(i=addition->places.begin(); i != addition->places.end(); i++) {
      if(places.find(i->first) != places.end()) {
	places[i->first]->first=min(places[i->first]->first,i->second->first);
	places[i->first]->second=max(places[i->first]->second,i->second->second);
      } else {
	places[i->first]=new pair<int,int>(i->second->first,i->second->second);
      }
    }
  }
  
  void print(ostream *out=cpout) {
    map<string,pair<int,int>*>::iterator i;
    for(i=places.begin(); i != places.end(); i++) {
      if(i!=places.begin()) *out << ", ";
      *out << i->first << ":" << i->second->first;
      if(i->second->first!=i->second->second) *out << "-" << i->second->second;
    }
  }
};

struct Error : Attribute {
  string msg;
  Error(string msg) { this->msg=msg; }
  void print(ostream *out=cpout) { *out << msg; }
};

// an boolean attribute that defaults to false, but is true when marked
struct MarkerAttribute : Attribute {
  bool inherit;
  MarkerAttribute(bool inherit) { this->inherit=inherit; }

  void print(ostream *out=cpout) { *out << "MARK"; }
  virtual Attribute* inherited() { return (inherit ? this : NULL); }
  void merge(Attribute *_addition)
  { inherit |= ((MarkerAttribute*)_addition)->inherit; }
};

struct IntAttribute : Attribute {
  int value;
  bool inherit;
  IntAttribute(bool inherit=false) { this->inherit=inherit; }

  void print(ostream *out=cpout) { *out << "MARK"; }
  virtual Attribute* inherited() { return (inherit ? this : NULL); }
  void merge(Attribute *_addition)
  { inherit |= ((MarkerAttribute*)_addition)->inherit; }
};

struct OrderAttribute : Attribute {
  static int max_id;
  int order;
  OrderAttribute() { order = max_id++; }
  OrderAttribute(int order) { this->order = order; }
  
  void print(ostream *out=cpout) { *out << order; }
  Attribute* inherited() { return new OrderAttribute(order); }
  void merge(Attribute *a) { order = MIN(order,((OrderAttribute*)a)->order); }
};


// By default, attributes that are passed around are *not* duplicated
struct CompilationElement {
  map<string,Attribute*> attributes; // should end up with null in default
  typedef map<string,Attribute*>::iterator att_iter;
  
  virtual void inherit_attributes(CompilationElement* src) {
    for(att_iter i=src->attributes.begin(); i!=src->attributes.end(); i++) {
      Attribute *a = src->attributes[i->first]->inherited();
      if(a!=NULL) {
	if(attributes.count(i->first)) {
	  attributes[i->first]->merge(a);
	} else {
	  attributes[i->first] = a;
	}
      }
    }
  }
  virtual string type_of() { return "CompilationElement"; }
  virtual bool isA(string c){ return (c=="CompilationElement")?true:false; }
  string to_str() { ostringstream s; print(&s); return s.str(); }
  virtual void print(ostream *out=cpout) {
    *out << pp_indent() << "Attributes [" << attributes.size() << "]\n";
    pp_push(2);
    for(att_iter i=attributes.begin(); i!=attributes.end(); i++) {
      *out<< pp_indent() << i->first <<": "; i->second->print(out); *out<<"\n";
    }
    pp_pop();
  }

  // for ordering in sets
  bool operator< (CompilationElement* e) {
    if(attributes.count("order") && e->attributes.count("order")) {
      return ((OrderAttribute*)attributes["order"])->order
        < ((OrderAttribute*)e->attributes["order"])->order;
    } else if(attributes.count("order")) { return true;
    } else if(e->attributes.count("order")) { return false;
    } else { return false; // when neither is ordered, all are equal
    }
  }
};

#endif // __COMPILER_UTILS__
