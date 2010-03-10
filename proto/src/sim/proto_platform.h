/* Declarations that customize the proto kernel for the simulator platform
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __PROTO_PLATFORM__
#define __PROTO_PLATFORM__

#ifdef __cplusplus
extern "C" {
#endif 

#define INLINE static inline
#define MAYBE_INLINE static inline
#define ATOMIC
#define VOID void

// At some point, the mote switching here needs to be removed
#ifdef IS_MOTE
#define are_exports_serialized 1
#define IS_COMPRESSED_COM_DATA 1
#define MAX_HOOD 4
#define MAX_SCRIPT_LEN 256L
#define MAX_SCRIPT_PKT 9
#define MAX_DIGEST_PKT 4
#define MAX_SCRIPTS 1
#define MAX_PROBES 1
#else
// Must be serialized to prevent "premature transmission" via pointers
#define are_exports_serialized 0
#define MAX_HOOD       64
  //#define MAX_HOOD       1000 // a massive hood size for high-volume experiments
#define MAX_SCRIPT_LEN (5000) // 24
#define MAX_SCRIPT_PKT 22
#define MAX_DIGEST_PKT 22
#define MAX_SCRIPTS 1
#define MAX_PROBES 3
#endif

#define NUM_SCRIPT_PKTS (MAX_SCRIPT_LEN / MAX_SCRIPT_PKT)
#define IS_WORD_ALIGN 0

#define MALLOC malloc

#define POST post

extern uint32_t mem_size;

extern int debug_id;
extern int is_debugging_val;
extern int is_tracing_val;
extern int is_script_debug_val;
extern int MAX_MEM_SIZE;

#ifdef __cplusplus
}
#endif 
#endif
