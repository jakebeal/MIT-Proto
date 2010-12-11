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

class CheckTypeConcreteness : public IRPropagator {
 public:
  CheckTypeConcreteness() : IRPropagator(true,false) { verbosity=0; }
  virtual void print(ostream* out=0) { *out << "CheckTypeConcreteness"; }
  virtual void act(Field* f);
};

#endif // __ANALYZER__
