/* Virtual machine functions supplied for use by platform operations
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __PROTO_VM__
#define __PROTO_VM__

#include "proto.h"

INLINE void NPOP (int n) { machine->sp -= n; }

INLINE void PUSH (DATA *val) { DATA_SET(machine->sp++, val); }
INLINE void VEC_PUSH (VEC_VAL *val) { VEC_SET(machine->sp++, val); }
INLINE void NUM_PUSH (NUM_VAL val) { NUM_SET(machine->sp++, val); }
INLINE void FUN_PUSH (FUN_VAL val) { FUN_SET(machine->sp++, val); }

INLINE DATA *POP (DATA *res) { return DATA_SET(res, --machine->sp); }
INLINE NUM_VAL NUM_POP (VOID) { return NUM_GET(--machine->sp); }
INLINE VEC_VAL *VEC_POP (VOID) { return VEC_GET(--machine->sp); }
INLINE FUN_VAL FUN_POP (VOID) { return FUN_GET(--machine->sp); }

INLINE DATA *ENV_PEEK (int off) { return machine->ep - off - 1; }
INLINE void ENV_PUSH (DATA *val) { DATA_SET(machine->ep++, val); }
INLINE void ENV_POP (int n) { machine->ep -= n; }

INLINE DATA *GLO_GET (uint16_t off) {
  // if (off >= machine->max_globals) uerror("GLOBALS OVERFLOW");
  return &machine->globals[off];
}

INLINE DATA *GLO_SET (uint16_t off, DATA *val) {
  DATA_SET(&machine->globals[off], val); return val;
}

INLINE DATA *PEEK (int off) { return machine->sp - off - 1; }
INLINE NUM_VAL NUM_PEEK (int off) { return NUM_GET(PEEK(off)); }
INLINE VEC_VAL *VEC_PEEK (int off) { return VEC_GET(PEEK(off)); }
INLINE FUN_VAL FUN_PEEK (int off) { return FUN_GET(PEEK(off)); }

INLINE uint8_t NXT_OP (MACHINE *m) { return m->script[m->pc++]; }
INLINE uint16_t NXT_OP16 (MACHINE *m) {
  return ((((uint16_t)(NXT_OP(m)))<<8) + ((uint16_t)(NXT_OP(m))));
}

#endif // __PROTO_VM
