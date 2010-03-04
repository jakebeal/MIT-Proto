/* ProtoKernel virtual machine
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// Note: to maintain compatibility with embedded devices, functions of
// no arguments must be declared function(void), not function()

#ifndef __PROTO__
#define __PROTO__

#ifdef __cplusplus
extern "C" {
#endif 

#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))
#endif

#include <string.h>
#include <math.h>
#include <stdint.h>

#ifndef INFINITY 
	#define INFINITY HUGE_VAL
#endif

/*
//Using math.h will give us a proper platform dependent definition of infinity 
//anyway. If it doesn't, it should be dealt in platform.h file.

#ifndef INFINITY
  #ifdef WIN32
    #define INFINITY (infinityf())
  #else
    // this is probably frowned upon
    #define INFINITY HUGE_VAL
  #endif
#endif
*/

#ifndef FLODEF
#define FLODEF
typedef float flo;
#endif

typedef int BOOL;
typedef float FLO;

// typedef uint8_t int;

typedef FLO TICKS;
typedef double TIME;
typedef int32_t MSECS;
typedef struct data_rec DATA;
typedef struct vec_val_rec VEC_VAL;
  // typedef struct tup_val_rec TUP_VAL; // not used
typedef FLO NUM_VAL;
typedef uint16_t FUN_VAL;
typedef uint32_t BIN_VAL;

#define TRUE 1
#define FALSE 0

#include "proto_opcodes.h"
#include "proto_platform.h"

typedef enum {
  NUM_TAG,
  VEC_TAG,
  FUN_TAG,
} TAG;

typedef union {
  NUM_VAL  n;
  VEC_VAL *v;
  FUN_VAL  f;
  BIN_VAL  b;
} DATA_VAL;

struct data_rec {
  uint8_t is_dead;
  uint8_t tag;
  DATA_VAL val;
};

#ifdef IS_COMPRESSED_COM_DATA
#ifndef __BIG_ENDIAN__ // normally, little endian
typedef struct {
  uint8_t is_dead:1, tag:2, extra:5;
  union {
    uint16_t n __attribute__ ((__packed__));
  } val;
} COM_DATA;  
#else
typedef struct {
  uint8_t extra:5, tag:2, is_dead:1;
  union {
    uint16_t n __attribute__ ((__packed__));
  } val;
} COM_DATA; 
#endif
#else
typedef struct data_rec COM_DATA;
#endif

struct vec_val_rec {
  int  n;
  int  cap;
  DATA elts[];
};

#define R_LED     0
#define G_LED     1
#define B_LED     2
#define ACT_ANGLE 3

#define N_ACTUATORS 4

#define LIGHT           0
#define SOUND           1
#define TEMPERATURE     2
#define USER3           3 // -> Conductivity is out:
#define SEN_ANGLE       4 // (No ADC, no buffer needed).

#define N_SENSORS 5

#define N_FOLDS 8

#define RADIO_EXPORT 0

#define BAD_VERSION 255

typedef struct {
  uint16_t id;
  uint8_t  timeout;
  flo      x, y, z;
  TICKS    stamp;
  TIME     time;
  DATA    *imports;
} NBR;

typedef struct {
  uint8_t is_exec;
  uint8_t is_open;
  DATA    data;
} STATE;

typedef struct {
  uint8_t  version;
  uint8_t  is_complete;
  uint8_t  bytes[MAX_SCRIPT_LEN];
  uint16_t len;
} SCRIPT;

typedef struct MACHINE {
  int16_t  id;
  uint8_t  timeout;
  uint8_t  cur_script;
  uint8_t  *script;
  SCRIPT   scripts[MAX_SCRIPTS];
  uint8_t  pkt_tracker[MAX_SCRIPT_LEN / MAX_SCRIPT_PKT + 1];
  uint8_t  pkt_listner[MAX_SCRIPT_LEN / MAX_SCRIPT_PKT + 1];
  BOOL     is_digest;
  uint8_t  radio_range;
  uint8_t  radio_range_sqr;
  NUM_VAL  x;
  NUM_VAL  y;
  NUM_VAL  z;
  NUM_VAL  nbr_x;
  NUM_VAL  nbr_y;
  NUM_VAL  nbr_z;
  NUM_VAL  nbr_lag;
  NUM_VAL  actuators[N_ACTUATORS];
  NUM_VAL  sensors[N_SENSORS];
  BOOL     is_folding[N_FOLDS];
  int      n_hood_vals; // number of neighborhood values
  uint16_t n_hood;      // number of neighbors
  COM_DATA *buf;
  COM_DATA *next_buf;
  NBR      hood_data[MAX_HOOD];
  NBR     *hood[MAX_HOOD];
  DATA    *hood_exports;
  TIME     last_time;
  TIME     time;
  TIME     period;
  TIME     desired_period;
  TICKS    ticks;
  TICKS    send_ticks;
  // EVAL MACHINERY
  DATA     res;
  uint16_t n_state;
  uint16_t n_globals;
  uint16_t max_globals;
  FUN_VAL  exec;
  STATE   *state;
  DATA    *globals;
  uint16_t n_stack;
  DATA    *stack;
  uint16_t n_env;
  DATA    *env;
  uint16_t pc;
  DATA    *sp;
  DATA    *ep;
  uint8_t *membuf;
  uint16_t buflen;
  uint16_t memlen;
  uint16_t memptr;
  uint16_t saved_memptr;
} MACHINE;

extern MACHINE *new_machine 
  (MACHINE *m, int id, int16_t x, int16_t y, int16_t z, 
   TIME desired_period, uint8_t *script, uint16_t script_len);

extern void open_machine (VOID);
extern void close_machine (VOID);
extern void export_machine (VOID);
extern void export_script (VOID);
extern void radio_receive_export
  (uint16_t src_id, uint8_t version, uint8_t timeout, flo x, flo y, flo z, uint8_t n, COM_DATA *buf);
extern int  radio_send_export (uint8_t version, uint8_t timeout, uint8_t n, uint8_t len, COM_DATA *buf);
extern void radio_receive_script_pkt (uint8_t version, uint16_t n, uint8_t pkt_num, uint8_t *script);
extern int  radio_send_script_pkt (uint8_t version, uint16_t n, uint8_t pkt_num, uint8_t *script);
extern void script_pkt_callback(uint8_t pkt_num);
extern int  serial_send_debug (uint8_t version, int len, uint8_t msgval);
extern void radio_receive_digest(uint8_t version, uint16_t script_len, uint8_t *digest);
extern int  radio_send_digest (uint8_t version, uint16_t script_len, uint8_t *digest);
  extern int script_export_needed(VOID);
extern uint16_t machine_mem_size (MACHINE *m);

extern MACHINE *machine;

// extern void post(char* string, ...);

extern void reinitHardware(void); // required for protobo

extern void mov (VEC_VAL *val);
extern void flex (NUM_VAL val);
extern void set_probe (DATA*, uint8_t);
extern void set_dt(NUM_VAL val);

extern NUM_VAL read_radio_range (VOID);
extern NUM_VAL read_bearing (VOID);
extern NUM_VAL read_speed (VOID);

extern void platform_operation(uint8_t op);

extern int is_throttling;
extern int depth;

// #define IS_TRACE

static inline DATA* init_num (DATA* x, NUM_VAL n) {
  x->tag = NUM_TAG;
  x->is_dead = FALSE;
  x->val.n = n;
  return x;
}

static inline NUM_VAL  NUM_VAL_GET(DATA_VAL *x) { return x->n; }
static inline NUM_VAL  NUM_VAL_SET(DATA_VAL *x, NUM_VAL n) { return x->n = n; }
static inline VEC_VAL *VEC_VAL_GET(DATA_VAL *x) { return x->v; }
static inline VEC_VAL *VEC_VAL_SET(DATA_VAL *x, VEC_VAL *v) { return x->v = v; }
static inline NUM_VAL NUM_GET(DATA *x) { return x->val.n; }
static inline NUM_VAL NUM_SET(DATA *x, NUM_VAL n) { x->is_dead = 0; x->tag = NUM_TAG; return x->val.n = n; }
static inline FUN_VAL FUN_GET(DATA *x) { return x->val.f; }
static inline FUN_VAL FUN_SET(DATA *x, FUN_VAL f) { x->is_dead = 0; x->tag = FUN_TAG; return x->val.f = f; }
static inline VEC_VAL *VEC_GET(DATA *x) { return x->val.v; }
static inline VEC_VAL *VEC_SET(DATA *x, VEC_VAL *v) { x->is_dead = 0; x->tag = VEC_TAG; return x->val.v = v; }
static inline DATA* DATA_SET(DATA *d, DATA *s) { 
  memcpy(d, s, sizeof(DATA));
  return d;
}

extern DATA *new_tup(int n, DATA *f);

extern void exec_machine (TICKS ticks, TIME time);

#define Null ((void*)0)

extern void uerror(const char* dstring, ...);
extern void POST(const char* string, ...);
extern void post_data_to (char *str, DATA *d);
extern void post_data (DATA *d);

extern void MEM_GROW (MACHINE *m);

#define PROTO_VERSION 402

#ifdef __cplusplus
}
#endif 

#endif
