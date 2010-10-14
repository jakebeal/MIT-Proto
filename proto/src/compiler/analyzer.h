/* Proto analyzer
Copyright (C) 2009, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// The analyzer takes us from an initial interpretation to a concrete,
// optimized structure that's ready for compilation

#ifndef __ANALYZER__
#define __ANALYZER__

#include "ir.h"

class Propagator : public CompilationElement {
 public:
  // behavior variables
  bool act_fields, act_ops, act_am;
  int verbosity;
  int loop_abort; // # equivalent passes through worklist before assuming loop
  // propagation work variables
  set<Field*, CompilationElement_cmp> worklist_f;
  set<OperatorInstance*, CompilationElement_cmp> worklist_o;
  set<AM*> worklist_a;
  bool any_changes;
  DFG* root;
  
  Propagator(bool field, bool op, bool am=false, int abort=10) 
    { act_fields=field; act_ops=op; act_am=am; loop_abort=abort; }
  bool propagate(DFG* g); // walk through worklist, acting until empty
  virtual void preprop() {} virtual void postprop() {} // hooks
  // action routines to be filled in by inheritors
  virtual void act(Field* f) {}
  virtual void act(OperatorInstance* oi) {}
  virtual void act(AM* am) {}
  // note_change: adds neighbors to the worklist
  void note_change(AM* am); void note_change(Field* f); 
  void note_change(OperatorInstance* oi);
  void maybe_set_range(Field* f,ProtoType* range); // change & note if different
 private:
  void queue_nbrs(AM* am, int marks=0); void queue_nbrs(Field* f, int marks=0);
  void queue_nbrs(OperatorInstance* oi, int marks=0);
};

class CheckTypeConcreteness : public Propagator {
 public:
  CheckTypeConcreteness() : Propagator(true,false) { verbosity=0; }
  virtual void print(ostream* out=0) { *out << "CheckTypeConcreteness"; }
  virtual void act(Field* f);
};

#endif // __ANALYZER__
