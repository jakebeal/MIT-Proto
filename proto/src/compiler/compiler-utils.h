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

#include <stdint.h>
#include <map>
#include <iostream>
#include <utility>
#include <string>
#include <sstream>
#include <vector>
#include "utils.h"
#include "nicenames.h"

using namespace std;

extern ostream *cpout, *cperr, *cplog; // Compiler output streams

/****** OUTPUT HANDLING ******/
struct CompilationElement;

/// Report a compiler internal error (i.e. not caused by user)
void ierror(string msg);
void ierror(CompilationElement *where, string msg);
/// pretty-print indenting
void pp_push(int n=1);
void pp_pop();
string pp_indent();

string b2s(bool b);
string f2s(float num, int precision=2);
string i2s(int num);

/********** ERROR REPORTING & VERBOSITY **********/
/*
 * Using globals is a little bit ugly, but we're not planning to have
 * the compiler be re-entrant any time soon, so it's fairly safe to assume
 * we'll always have precisely one compiler running.
 */
/// name of current phase, for error reporting
extern string compile_phase;
/// when true, terminate & exit gracefully
extern bool compiler_error;
/// when true, errors exit w. status 0
extern bool compiler_test_mode;
void compile_error(string msg);
void compile_error(CompilationElement *where,string msg);
void compile_warn(string msg);
void compile_warn(CompilationElement *where,string msg);
/// clean-up & kill application
void terminate_on_error();

// Standard levels for verbosity:
#define V1 if(verbosity>=1) *cpout // Major stages
#define V2 if(verbosity>=2) *cpout // Actions
#define V3 if(verbosity>=3) *cpout // Fine detail
#define V4 if(verbosity>=4) *cpout
#define V5 if(verbosity>=5) *cpout

/****** REFLECTION ******/

// Note: #t means the string literal for token t
#define reflection_base(t)                                              \
  virtual string type_of(){return #t;}                                  \
  virtual bool isA(string c) { return c==#t; }                          \
  string to_str() { ostringstream s; print(&s); return s.str(); }

#define reflection_sub(t,par)                                           \
  virtual string type_of() {return #t;}                                 \
  virtual bool isA(string c) { return c==#t||par::isA(c); }

#define reflection_sub2(t,par1,par2)                                    \
  virtual string type_of() {return #t;}                                 \
  virtual bool isA(string c) { return c==#t||par1::isA(c)||par2::isA(c); }


/****** COMPILATION ELEMENTS & ATTRIBUTES ******/
struct Attribute { reflection_base(Attribute);
  virtual void print(ostream *out=cpout) = 0;
  virtual Attribute* inherited() { return NULL; }
  virtual void merge(Attribute *addition) { 
    ierror("Attempt to merge incompatible attributes: " + this->to_str()
	   + ", " + addition->to_str());
  };
};


struct Context : Attribute { reflection_sub(Context,Attribute);
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

struct Error : Attribute { reflection_sub(Error,Attribute);
  string msg;
  Error(string msg) { this->msg=msg; }
  void print(ostream *out=cpout) { *out << msg; }
};

/**
 * A boolean attribute that defaults to false, but is true when marked
 */
struct MarkerAttribute : Attribute { reflection_sub(MarkerAttribute,Attribute);
  bool inherit;
  MarkerAttribute(bool inherit) { this->inherit=inherit; }

  void print(ostream *out=cpout) { *out << "MARK"; }
  virtual Attribute* inherited() { return (inherit ? this : NULL); }
  void merge(Attribute *_addition)
  { inherit |= ((MarkerAttribute*)_addition)->inherit; }
};

struct IntAttribute : Attribute { reflection_sub(IntAttribute,Attribute);
  int value;
  bool inherit;
  IntAttribute(bool inherit=false) { this->inherit=inherit; }

  void print(ostream *out=cpout) { *out << "MARK"; }
  virtual Attribute* inherited() { return (inherit ? this : NULL); }
  void merge(Attribute *_addition)
  { inherit |= ((MarkerAttribute*)_addition)->inherit; }
};


// By default, attributes that are passed around are *not* duplicated
#define CE CompilationElement
struct CompilationElement : public Nameable { reflection_base(CE);
  static uint32_t max_id;
  uint32_t elmt_id;
  map<string,Attribute*> attributes; // should end up with null in default
  typedef map<string,Attribute*>::iterator att_iter;
  
  CompilationElement() { elmt_id = max_id++; }
  virtual ~CompilationElement() {}
  virtual void inherit_attributes(CompilationElement* src) {
    if(src==NULL) ierror("Tried to inherit attributes from null source");
    for(att_iter i=src->attributes.begin(); i!=src->attributes.end(); i++) {
      Attribute *a = src->attributes[i->first]->inherited();
      if(a!=NULL) {
	if(attributes.count(i->first)) attributes[i->first]->merge(a);
        else attributes[i->first] = a;
      }
    }
  }
  // attribute utilities
  void clear_attribute(string a) 
  { if(attributes.count(a)) { delete attributes[a]; attributes.erase(a); } }
  bool marked(string a) { return attributes.count(a); }
  bool mark(string a) { attributes[a]=new MarkerAttribute(true); return true; }

  // typing and printing
  virtual void print(ostream *out=cpout) {
    *out << pp_indent() << "Attributes [" << attributes.size() << "]\n";
    pp_push(2);
    for(att_iter i=attributes.begin(); i!=attributes.end(); i++) {
      *out<< pp_indent() << i->first <<": "; i->second->print(out); *out<<"\n";
    }
    pp_pop();
  }
};

struct CompilationElement_cmp {
  bool operator()(const CompilationElement* ce1, 
                  const CompilationElement* ce2) const {
    return ce1->elmt_id < ce2->elmt_id;
  }
};
#define CEset(x) set<x,CompilationElement_cmp>
#define CEmap(x,y) map<x,y,CompilationElement_cmp>
#define ce2s(t) ((t)?(t)->to_str():"NULL")

/**
 * used for some indices
 */
struct CompilationElementIntPair_cmp {
  bool operator()(const pair<CompilationElement*,int> &ce1, 
                  const pair<CompilationElement*,int> &ce2) const {
    return ((ce1.first->elmt_id < ce2.first->elmt_id) ||
            (ce1.first->elmt_id == ce2.first->elmt_id &&
             ce1.second < ce2.second));
  }
};

#endif // __COMPILER_UTILS__
