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

extern void die (NUM_VAL val);
extern void clone_machine (NUM_VAL val);
extern VEC_VAL *read_coord_sensor (VOID);
extern VEC_VAL *read_ranger (VOID);
extern NUM_VAL read_sensor (uint8_t n);
extern NUM_VAL read_button (uint8_t n);
extern NUM_VAL read_slider (uint8_t ikey, uint8_t dkey, NUM_VAL init, NUM_VAL incr, NUM_VAL min, NUM_VAL max);
extern NUM_VAL read_light_sensor (VOID);
extern NUM_VAL read_microphone (VOID);
extern void    set_speak (NUM_VAL period);
extern NUM_VAL read_temp (VOID);
extern VEC_VAL *read_mouse_sensor (VOID);
extern NUM_VAL read_short (VOID);
extern void set_is_folding (BOOL val, int k);
extern BOOL read_fold_complete (int val);
extern void set_r_led (NUM_VAL val);
extern void set_g_led (NUM_VAL val);
extern void set_b_led (NUM_VAL val);
extern NUM_VAL radius_get (VOID);
extern NUM_VAL radius_set (NUM_VAL val);
extern NUM_VAL read_bump (VOID);
extern NUM_VAL set_channel (NUM_VAL diffusion, int k);
extern NUM_VAL read_channel (int k);
extern NUM_VAL drip_channel (NUM_VAL val, int k);
extern VEC_VAL *grad_channel (int k);
extern NUM_VAL cam_get (int k);

#ifdef __cplusplus
}
#endif 
#endif
