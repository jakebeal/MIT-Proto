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

/****** COMPILATION ELEMENTS & ATTRIBUTES ******/
struct Attribute {
  virtual Attribute* inherited() { return NULL; }
  virtual void merge(Attribute *addition) { 
    ierror("Attempt to merge incompatible attributes: " + this->to_str()
	   + ", " + addition->to_str());
  };
  string to_str() { ostringstream s; print(&s); return s.str(); }
  virtual void print(ostream *out=cpout) = 0;
};


struct Context : Attribute {
  map<string, pair<int,int>*> places; // file, <start,end>
  
  Context(string file_name,int line)
    { places[file_name] = new pair<int,int>(line,line); }
  
  Attribute* inherited() { return new Context(*this); }
  void merge(Attribute *_addition) {
    Context *addition = (Context*)_addition;
    map<string,pair<int,int>*>::iterator i;
    for(i=addition->places.begin(); i != addition->places.end(); i++) {
      if(places.find(i->first) != places.end()) {
	places[i->first]->first=min(places[i->first]->first,i->second->first);
	places[i->first]->second=max(places[i->first]->second,i->second->second);
      } else {
	places[i->first] = i->second;
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


// By default, attributes that are passed around are *not* duplicated
struct CompilationElement {
  map<string,Attribute*> attributes; // should end up with null in default
  typedef map<string,Attribute*>::iterator att_iter;
  
  virtual void inherit_attributes(CompilationElement* src) {
    for(att_iter i=src->attributes.begin(); i!=src->attributes.end(); i++) {
      Attribute *a = src->attributes[i->first]->inherited();
      if(a!=NULL) {
	if(attributes.find(i->first)!=attributes.end()) {
	  attributes[i->first]->merge(a);
	} else {
	  attributes[i->first] = a;
	}
      }
    }
  }
  virtual string type_of() { return "CompilationElement"; }
  string to_str() { ostringstream s; print(&s); return s.str(); }
  virtual void print(ostream *out=cpout) {
    *out << pp_indent() << "Attributes [" << attributes.size() << "]\n";
    pp_push(2);
    for(att_iter i=attributes.begin(); i!=attributes.end(); i++) {
      *out<< pp_indent() << i->first <<": "; i->second->print(out); *out<<"\n";
    }
    pp_pop();
  }
};

#endif // __COMPILER_UTILS__
