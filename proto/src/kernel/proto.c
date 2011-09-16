/* ProtoKernel virtual machine
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "proto.h"
#include "proto_vm.h"

#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))
#endif

#ifdef _WIN32
 #include <winsock.h> /* for ntohl() */
#endif

extern void * lookup_op_by_code (int code, char **name);

int is_throttling = 0; // TODO: are_exports_serialized must be true right now

int depth;

#define MAX_NBR_TIMEOUT  11
#define TIMEOUT_BASE 1.6

// INLINE uint8_t* word_align(uint8_t *x) {
//   return (uint8_t *)(((uint32_t)x + 3) & (~3));
// }

INLINE uint16_t word_align(uint16_t x) {
  return IS_WORD_ALIGN ? ((x + 3) & (~3)) : x;
}

uint16_t machine_mem_size (MACHINE *m) {
  //  return ((uint32_t)m->memptr) - ((uint32_t)m->membuf);
  return m->memptr;
}

uint8_t* PMALLOC(uint16_t sz) {
  uint8_t *ptr;
  MACHINE *m = machine;
  ATOMIC {
  if ((machine_mem_size(m)+sz) > m->memlen) 
    MEM_GROW(m);
  // post("M%d MALLOCING %d\n", m->id, sz);
  m->memptr = word_align(m->memptr);
  ptr = &m->membuf[m->memptr];
  m->memptr += sz;
  }
  return ptr;
}

void MARK_MEM (VOID) { 
  ATOMIC machine->saved_memptr = machine->memptr;
}

void FREE_MEM (VOID) { 
  ATOMIC machine->memptr = machine->saved_memptr;
}

INLINE BOOL is_tracing (MACHINE *m) {
  return is_tracing_val && (debug_id == -1 || debug_id == m->id);
}

INLINE BOOL is_debugging (MACHINE *m) {
  return is_debugging_val && (debug_id == -1 || debug_id == m->id);
}

INLINE BOOL is_script_debugging (MACHINE *m) {
  return is_script_debug_val && (debug_id == -1 || debug_id == m->id);
}

uint16_t timeout_of (uint8_t timeout_pow) {
  return (uint16_t)ceil(pow(TIMEOUT_BASE, timeout_pow));
}

int rnd (int min, int max) {
  return (rand() % (max - min + 1)) + min;
}

NUM_VAL num_rnd (NUM_VAL min, NUM_VAL max) {
  FLO val = (double)rand() / (double)RAND_MAX;
  return (val * (max - min)) + min;
}

static DATA *new_num(NUM_VAL i) {
  return init_num((DATA*)PMALLOC(sizeof(DATA)), i);
}

MAYBE_INLINE DATA* init_vec (DATA* x, int n, int cap, DATA *f) {
  size_t i;
  int len = sizeof(VEC_VAL)+cap*sizeof(DATA);
  VEC_VAL *dat = (VEC_VAL*)PMALLOC(len);
  dat->n   = n;
  dat->cap = cap;
  for (i = 0; i < n; i++)
    DATA_SET(&dat->elts[i], f);
  x->val.v   = dat;
  x->is_dead = FALSE;
  x->tag     = VEC_TAG;
  return x;
}

VEC_VAL *ensure_vec(DATA *dat, int n) {
  VEC_VAL *v = VEC_GET(dat);
  if (dat->tag != VEC_TAG) {
    DATA val; init_num(&val, 0);
    init_vec(dat, n, n, &val);
    return VEC_GET(dat);
  } else if (n < v->n) {
    if (n <= v->cap) {
      v->n = n;
    } else {
      DATA val; init_num(&val, 0);
      init_vec(dat, n, n, &val);
    }
    return VEC_GET(dat);
  } else
    return v;
}

DATA *data_copy (DATA *dst, DATA *src) {
  dst->is_dead = src->is_dead;
  switch (src->tag) {
  case NUM_TAG: 
    DATA_SET(dst, src++);
    break; 
  case VEC_TAG: {
    VEC_VAL *vs = VEC_GET(src);
    int n       = vs->n;
    VEC_VAL *vd = ensure_vec(dst, n);
    int i;
    for (i = 0; i < n; i++)
      data_copy(&vd->elts[i], &vs->elts[i]);
    break; }
  default:
    uerror("M%d UNKNOWN TAG %d\n", machine->id, src->tag);
  }
  return src;
}

VEC_VAL* grow_vec(VEC_VAL *v, int cap) {
  int i, len = sizeof(VEC_VAL)+cap*sizeof(DATA);
  VEC_VAL *dat = (VEC_VAL*)PMALLOC(len);
  dat->n   = v->n;
  dat->cap = cap;
  for (i = 0; i < v->n; i++)
    data_copy(&dat->elts[i], &v->elts[i]);
  POST("M%d GROW VEC %lx %d\n", machine->id, v, v->n);
  // FREE(v);
  return dat;
}

MAYBE_INLINE void ensure_len(VEC_VAL *v, int n) {
  if (v->n != n) {
    if (n > v->cap) 
      uerror("TOO FEW ELEMENTS %d > %d\n", n, v->cap);
    v->n = n;
  } 
}

static DATA *new_vec(int n, DATA *f) {
  return init_vec((DATA*)PMALLOC(sizeof(DATA)), n, n, f);
}

DATA *new_tup(int n, DATA *f) {
  return new_vec(n, f);
}

INLINE int vec_len (VEC_VAL *vec) {
  return vec->n;
}

INLINE void vec_zap (VEC_VAL *vec) {
  vec->n = 0;
}

MAYBE_INLINE VEC_VAL* vec_add (VEC_VAL *vec, DATA *val) {
  data_copy(&vec->elts[vec->n++], val);
  if (vec->n > vec->cap) 
    vec = grow_vec(vec, 2*vec->cap);
  return vec;
}

INLINE void init_data (DATA *dat, TAG tag, DATA_VAL val) {
  dat->is_dead = FALSE;
  dat->tag     = tag;
  dat->val     = val;
}

MACHINE *machine;

#define NBR_NULL -1

INLINE void nbr_invalid_set (NBR *nbr) {
  nbr->stamp = NBR_NULL;
}

INLINE void nbr_valid_set (NBR *nbr) {
  nbr->stamp = 0;
}

INLINE BOOL is_nbr_valid (NBR *nbr) {
  return nbr->stamp != NBR_NULL;
}

void del_nbr (int k) {
  int i;
  MACHINE *m = machine;
  nbr_invalid_set(m->hood[k]);
  for (i = k; i < (m->n_hood-1); i++)
    m->hood[i] = m->hood[i+1];
  m->n_hood -= 1;
}

void ins_nbr (NBR *nbr, int k) {
  MACHINE *m = machine;
  int i;
  for (i = m->n_hood; i > k; i--)
    m->hood[i] = m->hood[i-1];
  m->hood[k] = nbr;
  m->n_hood += 1;
}

void post_nbrs (MACHINE *m) {
  int i;
  for (i = 0; i < m->n_hood; i++) 
    POST("M%d|%lx ", m->hood[i]->id, m->hood[i]);
}

NBR *add_nbr (uint16_t src_id) {
  int i, j;
  MACHINE *m = machine;
  NBR *nbr;
  for (i = 0; i < MAX_HOOD; i++) {
    nbr = &m->hood_data[i];
    if (!is_nbr_valid(nbr)) {
      nbr->id = src_id;
      nbr->stamp = m->ticks;
      nbr->time  = m->time;
      for (j = 0; j < m->n_hood; j++) 
        if (src_id < m->hood[j]->id) {
          break;
        }
      ins_nbr(nbr, j);
      // POST("M%d ADDING NBR %d: ", m->id, src_id); post_nbrs(m); POST("\n");
      return nbr;
    }
  }
  return NULL;
}

MAYBE_INLINE NBR *find_nbr (uint16_t src_id) {
  MACHINE *m = machine;
  int left  = 0;
  int right = m->n_hood-1;
  // POST("M%d LOOKING UP NBR %d N_HOOD %d: ", m->id, src_id, m->n_hood);
  // post_nbrs(m); POST("\n");
  while (left <= right) {
    int mid = (left + right)>>1;
    NBR *nbr = m->hood[mid];
    // POST("M%d LOOKING AT %d M%d\n", m->id, mid, nbr->id);
    if (nbr->id == src_id) {
      // POST("M%d FOUND NBR AT %d\n", mid);
      return nbr;
    } else if (src_id < nbr->id)
      right = mid - 1;
    else
      left  = mid + 1;
  }
  // POST("M%d LOST NBR %d\n", m->id, src_id);
  return NULL;
}

extern void link_script (MACHINE *m);
extern void install_script(MACHINE *m, uint8_t *script, uint8_t slot, uint16_t len, uint8_t version);
extern void install_script_pkt(MACHINE *m, uint8_t pkt_num, uint8_t *script_pkt, uint16_t len);
void clear_pkt_tracker(void);
uint8_t add_pkt(uint8_t pkt_num);
int is_script_complete(void);

#define TRICKLE_BASE_TIME 5
#define TRICKLE_MAX_WAIT 100
uint16_t trickle_wait_time = 0;
uint16_t trickle_passed_time = 0;

void reinit_machine (MACHINE *m) {
  int i;
  m->n_hood       = 0;
  for (i = 0; i < N_SENSORS; i++)
    m->sensors[i] = 0;
  for (i = 0; i < MAX_HOOD; i++)
    nbr_invalid_set(&m->hood_data[i]);
  reinitHardware();
}

MACHINE *init_machine
   (MACHINE *m, int id, int16_t x, int16_t y, int16_t z, 
    TIME desired_period, uint8_t *script, uint16_t script_len) {
  int i;
  m->id           = id;
  m->cur_script   = 0;
  for (i = 0; i < MAX_SCRIPTS; i++) {
    m->scripts[i].version = 0;
    m->scripts[i].is_complete = FALSE;
  }
  install_script(m, script, m->cur_script, script_len, m->scripts[m->cur_script].version);
  trickle_wait_time = TRICKLE_BASE_TIME/2 + (rand()% TRICKLE_BASE_TIME /2);
  m->is_digest    = 1;
  m->radio_range  = read_radio_range();
  m->radio_range_sqr = m->radio_range * m->radio_range;
  m->x            = x;
  m->y            = y;
  m->z            = z;
  m->ticks        = 0;
  m->time         = 0;
  m->last_time    = 0;
  m->send_ticks   = 0;
  m->timeout      = 0;
  m->period       = m->desired_period = desired_period;
  // m->next_id   = 0;
  reinit_machine(m);
  return m;
}

DATA nul_tup_dat;

MACHINE *new_machine
    (MACHINE *m, int id, int16_t x, int16_t y, int16_t z, 
     TIME desired_period, uint8_t *script, uint16_t script_len) {
  // MACHINE *m = (MACHINE*)PMALLOC(sizeof(MACHINE));
  machine = m;
  init_vec(&nul_tup_dat, 0, 0, NULL);
  m = init_machine(m, id, x, y, z, desired_period, script, script_len);
  MARK_MEM();
  return m;
}

INLINE DATA *STATE_GET (uint8_t i) {
  // if (i >= machine->n_state) uerror("STATE OVERFLOW");
  return &machine->state[i].data;
}

MAYBE_INLINE DATA *STATE_SET (uint8_t i, DATA *val) {
  DATA_SET(&machine->state[i].data, val);
  return val;
}

INLINE BOOL IS_EXEC (uint8_t i) {
  return machine->state[i].is_exec;
}

INLINE BOOL IS_EXEC_SET (uint8_t i, BOOL val) {
  return machine->state[i].is_exec = val;
}

INLINE BOOL IS_OPEN (uint8_t i) {
  return machine->state[i].is_open;
}

INLINE BOOL IS_OPEN_SET (uint8_t i, BOOL val) {
  return machine->state[i].is_open = val;
}

////
//// VIRTUAL MACHINE
////

// shared VM functions in proto_vm.h

extern DATA *eval(DATA *res, FUN_VAL fun);

MAYBE_INLINE DATA funcall0 (FUN_VAL fun) {
  DATA res;
  FUN_PUSH(machine->pc);
  eval(&res, fun);
  return res;
}

MAYBE_INLINE DATA funcall1 (FUN_VAL fun, DATA *arg) {
  DATA res;
  FUN_PUSH(machine->pc);
  ENV_PUSH(arg);
  eval(&res, fun);
  ENV_POP(1);
  return res;
}

MAYBE_INLINE DATA funcall2 (FUN_VAL fun, DATA *a1, DATA *a2) {
  DATA res;
  FUN_PUSH(machine->pc);
  ENV_PUSH(a1); ENV_PUSH(a2);
  eval(&res, fun);
  ENV_POP(2);
  return res;
}

void clear_actuators (VOID) {
  MACHINE *m = machine;
  int i;
  for (i = 0; i < N_ACTUATORS; i++)
    m->actuators[i] = 0;
}

void clear_exports (VOID) {
  MACHINE *m = machine;
  int i;
  for (i = 0; i < m->n_hood_vals; i++) 
    m->hood_exports[i].is_dead = TRUE;
}
  
void maybe_hibernate_machine (VOID) {
  int i;
  for (i = 0; i < machine->n_state; i++) {
    if (!IS_EXEC(i)) 
      IS_OPEN_SET(i, 0);
    IS_EXEC_SET(i, 0);
  }
}
  
void clear_is_exec (VOID) {
  int i;
  for (i = 0; i < machine->n_state; i++) {
    IS_EXEC_SET(i, 0);
  }
}
  
void clear_is_open (VOID) {
  int i;
  for (i = 0; i < machine->n_state; i++) {
    IS_OPEN_SET(i, 0);
  }
}
  
void open_machine (VOID) {
  //MACHINE *m = machine;
  // m->n_hood      = 0;
  // m->next_id     = 0;
  // for (i = 0; i < N_SENSORS; i++)
  //   m->sensors[i] = 0;
  clear_actuators();
  clear_is_exec();
  clear_is_open();
  // POST("M%d HOOD VALUES %d\n", m->id, m->n_hood_vals);
}

void close_machine (VOID) {
  MACHINE *m = machine;
  m->n_hood_vals = 0;
}

typedef union {
  float    f;
  uint32_t i;
} FLOint;

#if IS_COMPRESSED_COM_DATA
INLINE NUM_VAL NUM_DECODE_VAL (COM_DATA *src) {
  FLOint fi;
  fi.i = ((uint32_t)(ntohs(src->val.n))) << 16;
  return fi.f;
}
INLINE uint16_t NUM_ENCODE_VAL (DATA *src) {
  FLOint fi;
  fi.f = src->val.n;
  return htons(fi.i >> 16);
}
#else
INLINE NUM_VAL NUM_DECODE_VAL (COM_DATA *src) {
  return src->val.n;
}
INLINE NUM_VAL NUM_ENCODE_VAL (DATA *src) {
  return src->val.n;
}
#endif

INLINE void NUM_DECODE (DATA *dst, COM_DATA *src) {
  dst->is_dead = src->is_dead;
  dst->tag     = src->tag;
  dst->val.n   = NUM_DECODE_VAL(src);
}


INLINE void NUM_ENCODE (COM_DATA *dst, DATA *src) {
  dst->is_dead = src->is_dead;
  dst->tag     = src->tag;
  dst->val.n   = NUM_ENCODE_VAL(src);
}

COM_DATA *data_decode (DATA *dst, COM_DATA *src) {
  switch (src->tag) {
  case NUM_TAG: 
    NUM_DECODE(dst, src++);
    break; 
  case VEC_TAG: {
    int is_dead = src->is_dead;
    int n = (int)NUM_DECODE_VAL(src++);
    VEC_VAL *v = ensure_vec(dst, n);
    int i;
    for (i = 0; i < n; i++)
      src = data_decode(&v->elts[i], src++);
    dst->is_dead = is_dead;
    break; }
  default:
    if (src->is_dead)
      NUM_DECODE(dst, src++);
    else
      uerror("M%d UNKNOWN TAG %d\n", machine->id, src->tag);
  }
  return src;
}

COM_DATA *data_encode (COM_DATA *dst, DATA *src, int *len) {
  *len += 1;
  switch (src->tag) {
  case NUM_TAG: 
    NUM_ENCODE(dst++, src); 
    break; 
  case VEC_TAG: {
    VEC_VAL *v = src->val.v;
    int n      = v->n;
    int i;
    DATA dat;
    dat.is_dead = src->is_dead;
    dat.tag = VEC_TAG; 
    dat.val.n = n;
    NUM_ENCODE(dst, &dat);
    dst++;
    for (i = 0; i < n; i++)
      dst = data_encode(dst, &v->elts[i], len);
    break; }
  default:
    if (src->is_dead)
      NUM_ENCODE(dst++, src);
    else
      uerror("M%d UNKNOWN TAG %d\n", machine->id, src->tag);
  }
  return dst;
}

INLINE int ABS(int x) { return (x < 0) ? -x : x; }

#define NBR_TIMEOUT 10  // Number of missed messages before punting.
// #define NBR_TIMEOUT 100000

void prune_hood (MACHINE *dst) {
  int i,x;
  TICKS newest = 0;
  // for (i = 0; i < dst->n_hood; i++) 
  //   newest = MAX(newest, dst->hood[i]->stamp);
  newest = dst->ticks;
  for (i = 0; i < dst->n_hood; i++) {
    NBR *nbr = dst->hood[i];
    // Buggy way of doing it: would wait as long as 1.6^{MAX_NBR_TIMEOUT + NBR_TIMEOUT}
    //uint16_t nbr_timeout
    //  = timeout_of(nbr->timeout + NBR_TIMEOUT) - timeout_of(nbr->timeout);
    // Timeout = Sum_{x=t+1}^{t+NBR_TIMEOUT}(timeout_of(MIN(x,MAX_NBR_TIMEOUT))
    uint16_t nbr_timeout = 0;
    if (is_throttling)
      for(x=1;x<=NBR_TIMEOUT;x++)
         nbr_timeout += timeout_of(MIN(nbr->timeout+x,MAX_NBR_TIMEOUT));
    else
      nbr_timeout = NBR_TIMEOUT;
    if ((newest - nbr->stamp) > nbr_timeout) {
      // POST("M%d DEL NBR %d: ", dst->id, nbr->id); post_nbrs(dst); POST("\n");
      del_nbr(i);
    }
  }
}

void radio_receive_export
    (uint16_t src_id, uint8_t version, uint8_t timeout, flo x, flo y, flo z, uint8_t n, COM_DATA *buf) {
  int i;
  COM_DATA *bp = buf;
  MACHINE *dst = machine;
  int is_debug = is_debugging(dst);

  if (version == dst->scripts[dst->cur_script].version && 
      n == dst->n_hood_vals) {
    NBR *nbr = find_nbr(src_id);
    if (nbr == NULL) {
      if (is_debug) POST("M%d ADDING NBR %d\n", dst->id, src_id);
      nbr = add_nbr(src_id);
      if (nbr == NULL) {
        return; // TODO: LRU
      } else {
        if (is_debug) POST("M%d ADDED %d V%d\n", dst->id, src_id, version);
      } 
    } // else
    // if (is_debug) POST("M%d FOUND NBR %d\n", dst->id, src_id);
    nbr->stamp   = dst->ticks;
    nbr->timeout = timeout;
    nbr->time    = dst->time;
    nbr->x       = x;
    nbr->y       = y;
    nbr->z       = z;
    // if (n > 1) POST("M%d RCVEXP[%d](%d):", dst->id, src_id, n-1);
    if (is_debug) POST("M%d<-M%d %d %d IMPORTS ", dst->id, nbr->id, n, dst->n_hood_vals);
    if (are_exports_serialized) {
      for (i = 0; i < n; i++) {
         bp = data_decode(&nbr->imports[i], bp);
         if (is_debug) { POST("/%d/", i); post_data(&nbr->imports[i]); POST(" "); }
         // POST(" %d ", dst->hood_exports[i].tag); post_data(&nbr->imports[i]);
      }
    } else
      nbr->imports = (DATA*)buf;
    if (is_debug) POST("\n");
    // if (n > 1) POST("\n");
    // else
    // POST("ERROR %d<-%d DIFF NUMBER HOOD VALS %d VS %d\n", 
    //      dst->id, src_id, n, dst->n_hood_vals+1);
  } else {
    if (is_debug) { POST("BAD VERSION %d OR N %d N_HOOD_VALS %d\n", version, n, dst->n_hood_vals); }
  }
}

void hood_send (MACHINE *m, COM_DATA *buf, int n, COM_DATA *bp) {
  if (is_throttling) {
    BOOL is_timeout = ((m->ticks - m->send_ticks) > timeout_of(m->timeout));
    BOOL is_fresh   = (memcmp(m->next_buf, m->buf, m->buflen) != 0);
    if (is_timeout || is_fresh) {
      radio_send_export(m->scripts[m->cur_script].version, m->timeout, n, bp-buf, buf);
      memcpy(m->buf, m->next_buf, m->buflen);
      if (is_fresh || !is_throttling)
         m->timeout = 0;
      else
         m->timeout = MIN(m->timeout + 1, MAX_NBR_TIMEOUT); 
      m->send_ticks = m->ticks;
    }
  } else {
    radio_send_export(m->scripts[m->cur_script].version, m->timeout, n, bp-buf, buf);
    m->send_ticks = m->ticks;
  }
}

void export_machine (VOID) {
  MACHINE *m = machine;
  COM_DATA *buf, *bp;
  int n, len, is_debug;
  if (are_exports_serialized) {
    buf = m->next_buf;
    bp  = buf;
    is_debug = is_debugging(m);
    len = 0;
    memset(buf, 0, m->buflen);
    // POST("SETTING TICKS %d\n", k);
    if (is_debug) POST("M%d EXPORTS ", m->id);
    for (n = 0; n < m->n_hood_vals; n++) {
      DATA *out = &m->hood_exports[n];
      bp = data_encode(bp, out, &len);
      if (is_debug) { POST("<"); post_data(out); POST("> "); }
      // POST("EXPORTING[%d/%d/%d/%d] ", i, m->n_hood_vals, k, m->scripts[m->cur_script].version);
      // post_data(&m->hood_exports[i]);
      // POST(" ");
      // post_data(&buf[k-1]);
      // POST("\n");
    }
    
    if (is_debug) POST("\n");
    // POST("M%d HV %d N %d\n", m->id, m->n_hood_vals, bp-buf);
    // POST("EXPORT SIZE  %d == %d\n", (bp-buf)*sizeof(DATA), m->buflen);
    if (((bp-buf)*sizeof(DATA)) > (m->buflen)) {
      POST("EXPORT SIZE TOO BIG %d x %d > %d\n", len, m->buflen, sizeof(DATA));
    }
  } else {
    bp = buf = (COM_DATA*)m->hood_exports;
    n = m->n_hood_vals;
  }
  hood_send(m, buf, n, bp);
}

MAYBE_INLINE uint16_t num_pkts (uint16_t len) {
  return (uint16_t)ceil(len / (float)MAX_SCRIPT_PKT);
}

void dump_byte (uint8_t byte, uint16_t n_bits) {
  int j;
  for (j = 0; j < n_bits; j++)
    POST("%d", (byte>>j)&1);
}

void dump_digest (uint8_t *digest, uint16_t len) {
  uint8_t i;
  uint8_t npkts = num_pkts(len);
  uint8_t major = npkts / 8;
  uint8_t minor = npkts % 8;
  // POST("L%d/P%d/J%d/N%d-", len, npkts, major, minor);
  for (i = 0; i < major; i++) 
    dump_byte(digest[i], 8);
  dump_byte(digest[major], minor);
}

void needed_and_available_pkts
    (uint8_t *needed, uint8_t *nbr_digest, uint8_t *digest, uint16_t len) {
  uint8_t i;
  uint8_t npkts = num_pkts(len);
  uint8_t major = npkts / 8;
  // uint8_t minor = npkts % 8;
  // POST("LEN %d NPKTS %d MAJOR %d MINOR %d\n", len, npkts, major, minor);
  for (i = 0; i < (major+1); i++) {
    needed[i] |= ~nbr_digest[i] & digest[i];
  }
}

void radio_receive_digest(uint8_t version, uint16_t script_len, uint8_t *digest) {
  MACHINE *m = machine;
  SCRIPT *cur_script = &m->scripts[m->cur_script];
  SCRIPT *nxt_script = &m->scripts[(m->cur_script+1)%MAX_SCRIPTS];
  if (version > cur_script->version) {
    if (version > nxt_script->version) {
      if (is_script_debugging(m)) 
         POST("M%d NEW VERSION %d LEN %d---\n", m->id, version, script_len);
      clear_pkt_tracker();
      nxt_script->is_complete = FALSE;
      nxt_script->version     = version;
      nxt_script->len         = script_len;
    }
  }
  needed_and_available_pkts(m->pkt_listner, digest, m->pkt_tracker, script_len);
  if (is_script_debugging(m)) {
    POST("---M%d HAS VERSION %d LEN %d---\n", m->id, version, script_len);
    POST("NEEDED: "); dump_digest(m->pkt_listner, script_len); POST("\n");
    POST("HEARD:  "); dump_digest(digest, script_len); POST("\n");
    POST("HAVE:   "); dump_digest(m->pkt_tracker, script_len); POST("\n");
  }
}

void radio_receive_script_pkt (uint8_t version, uint16_t n, uint8_t pkt_num, uint8_t *script_pkt) {
  MACHINE *m = machine;
  SCRIPT *cur_script = &m->scripts[m->cur_script];
  SCRIPT *nxt_script = &m->scripts[(m->cur_script+1)%MAX_SCRIPTS];

  
  if (is_script_debugging(m))
    POST("M%d RCV PKT %d VER %d LEN %d CUR %d NXT %d DON %d\n", 
         m->id, pkt_num, version, n, cur_script->version, nxt_script->version,
         cur_script->is_complete);
  if (version > cur_script->version || (MAX_SCRIPTS == 1 && !cur_script->is_complete)) {
    if (version == nxt_script->version) {
      if (is_script_debugging(m))
        POST("M%d GOT PKT %d VER %d LEN %d\n", m->id, pkt_num, version, n);
      install_script_pkt(m, pkt_num, script_pkt, n);
    } else if (version > nxt_script->version) {
      if (is_script_debugging(m)) {
        POST("M%d NEW VERSION\n", m->id);
        POST("M%d GOT PKT %d VER %d LEN %d\n", m->id, pkt_num, version, n);
      }
      clear_pkt_tracker();
      nxt_script->version     = version;
      nxt_script->is_complete = FALSE;
      install_script_pkt(m, pkt_num, script_pkt, n);
    }
    if (is_script_complete()) {
      m->cur_script = (m->cur_script+1)%MAX_SCRIPTS;
      if (is_script_debugging(m)) 
        POST("M%d PACKET COMPLETE LEN %d\n", m->id, nxt_script->len);
      m->scripts[m->cur_script].is_complete = TRUE;
      // clear_pkt_tracker();
      link_script(m);

      ATOMIC {
        FREE_MEM();
        open_machine();
      }

    }
  }
}

void export_script (VOID) {
  MACHINE *m = machine;
  size_t i;
  //  char OutputMsg[80];

/*   ++trickle_passed_time; */
/*   if(trickle_passed_time > trickle_wait_time){ */
/*     trickle_passed_time = 0; */
/*     trickle_wait_time = MAX(trickle_wait_time/2, TRICKLE_MAX_WAIT /2) */
/*       + (rand()% MAX(trickle_wait_time/2, TRICKLE_MAX_WAIT /2)); */
/*    }    */

  SCRIPT *nxt_script;
  
  if (m->scripts[(m->cur_script+1)%MAX_SCRIPTS].version > m->scripts[m->cur_script].version)
    nxt_script = &m->scripts[(m->cur_script+1)%MAX_SCRIPTS];
  else
    nxt_script = &m->scripts[m->cur_script];

  for (i = 0; i < num_pkts(nxt_script->len); i++) {
    uint8_t major = i / 8;
    uint8_t minor = i % 8;

    if (m->pkt_listner[major] & (1 << minor)) {
      
      if (is_script_debugging(m))
         POST("M%d SND PKT %d VER %d LEN %d\n", 
              m->id, i, m->scripts[m->cur_script].version, m->scripts[m->cur_script].len);
      radio_send_script_pkt(nxt_script->version, nxt_script->len,
                            i, &nxt_script->bytes[i * MAX_SCRIPT_PKT]);
      return;
    }
  }
  if (is_script_debugging(m)) {
    POST("M%d DIGEST: ", m->id); dump_digest(m->pkt_tracker, nxt_script->len); POST("\n");
  }
  radio_send_digest(nxt_script->version, nxt_script->len, m->pkt_tracker);
}

void script_pkt_callback(uint8_t pkt_num){
  MACHINE *m = machine;
  uint8_t major = pkt_num / 8;
  uint8_t minor = pkt_num % 8;
  ATOMIC m->pkt_listner[major] &= ~(1 << minor);
}

void exec_machine (TICKS ticks, TIME time) {
  MACHINE *m = machine;
  SCRIPT  *s = &m->scripts[m->cur_script];
  #ifdef IS_MOTE
  m->last_time = m->time;
  m->time      = time;
  m->ticks     = ticks;
  #endif
  if (s->is_complete) {
    #ifndef IS_MOTE
    m->last_time = m->time;
    m->time      = time;
    m->ticks     = ticks;
    #endif
    if (is_debugging(m) || is_tracing(m)) { 
      depth = 0;
      POST("M%d EXEC %f V%d\n", m->id, m->ticks, s->version);
    }
    clear_actuators();
    clear_exports();
    prune_hood(m);
    m->res = funcall0(m->exec);
    maybe_hibernate_machine();
  }
}

INLINE uint8_t n_hood (VOID) {
  return machine->n_hood;
}

//
// VECTOR OPERATIONS
//

//Create Tuples
INLINE void tup_exec (int n, int offset) { 
  int i;
  DATA *data = GLO_GET(offset);
  VEC_VAL *v = VEC_GET(data); 
  for (i = 0; i < n; i++)
    DATA_SET(&v->elts[i], PEEK(n-i-1));
  NPOP(n); PUSH(data);
}

//Access Elements of Vectors
INLINE void elt_exec (VOID) {
  VEC_VAL *val = VEC_PEEK(1);
  NUM_VAL k    = NUM_PEEK(0);
  NPOP(2); PUSH(vec_elt(val, (int)k));
}

//Hook for getting Length of Vectors
INLINE void len_exec(void) {
  VEC_VAL *v1 = VEC_POP();
  NUM_PUSH(v1->n);
} 

//Vector addition: the shorter vector has elements padded with 0 to match len
void vadd_exec (int off) {
  int i;
  VEC_VAL *vr = VEC_GET(GLO_GET(off));
  VEC_VAL *v0 = VEC_PEEK(1);
  VEC_VAL *v1 = VEC_PEEK(0);
  int n0      = v0->n;
  int n1      = v1->n;
  int nMin    = MIN(n0,n1);
  int nMax    = MAX(n0,n1);
  VEC_VAL *vlarger = (n0 == nMax) ? v0 : v1;
  ensure_len(vr, nMax);

  for (i = 0; i < nMin; i++) {
    NUM_SET(vec_elt(vr,i), NUM_GET(vec_elt(v0,i)) + NUM_GET(vec_elt(v1, i)));
  }
  
  for (i = nMin; i < nMax; i++) {
    NUM_SET(vec_elt(vr,i), NUM_GET(vec_elt(vlarger,i)));
  }  
  
  NPOP(2); VEC_PUSH(vr);
}

void vsub_exec (int off) {
  int i;
  VEC_VAL *vr = VEC_GET(GLO_GET(off));
  VEC_VAL *v0 = VEC_PEEK(1);
  VEC_VAL *v1 = VEC_PEEK(0);
  int n0      = v0->n;
  int n1      = v1->n;
  int nMin    = MIN(n0, n1);
  int nMax    = MAX(n0, n1);
  ensure_len(vr, nMax);

  VEC_VAL *vlarger = (n0 == nMax) ? v0 : v1;
  int sign = (n0 == nMax) ? 1 : -1; /*If v1 is longer than v0, padding means
  that we're subtracting elts of v1 from "0". So -ve.
  */
  
  for (i = 0; i < nMin; i++) {
    NUM_SET(vec_elt(vr, i), NUM_GET(vec_elt(v0, i)) - NUM_GET(vec_elt(v1, i)));
  }
  for (i = nMin; i < nMax; i++) {
    NUM_SET(vec_elt(vr, i), sign * NUM_GET(vec_elt(vlarger,i)));
  }
  
  NPOP(2); VEC_PUSH(vr);
}

//Scalar Multiplication
void vmul_exec (int off) {
  VEC_VAL *vr = VEC_GET(GLO_GET(off));
  NUM_VAL  n  = NUM_PEEK(1);
  VEC_VAL *v1 = VEC_PEEK(0);
  int i;
  int n1      = v1->n;
  ensure_len(vr, n1);
  
  for (i = 0; i < n1; i++) 
    NUM_SET(vec_elt(vr, i), n * NUM_GET(vec_elt(v1, i)));

  NPOP(2); VEC_PUSH(vr);
}

void vdot_exec (VOID) {
  NUM_VAL val = 0;
  VEC_VAL *v0 = VEC_PEEK(1);
  VEC_VAL *v1 = VEC_PEEK(0);
  int i;
  int n0      = vec_len(v0);
  int n1      = vec_len(v1);
  int nr      = MIN(n0, n1);
  for (i = 0; i < nr; i++) 
    val += NUM_GET(vec_elt(v0, i)) * NUM_GET(vec_elt(v1, i));
  NPOP(2); NUM_PUSH(val);
}

/*
Slicing is designed to work like in python.
*/
void vslice_exec (int off) {
  //TODO: When we have optional arguments, add "step" as optional argument
  VEC_VAL *v1 = VEC_PEEK(0);
  NUM_VAL len = vec_len(v1);
  //Negative Indices are taken to mean len + index.
  NUM_VAL start = NUM_PEEK(1) >= 0 ? NUM_PEEK(1): len + NUM_PEEK(1);
  NUM_VAL stop = NUM_PEEK(2) >= 0 ? NUM_PEEK(2): len + NUM_PEEK(2);
  //TODO: Check if stop index is greater than len
  VEC_VAL *vr = VEC_GET(GLO_GET(off));
  int ans_len = stop - start;
  ensure_len(vr, ans_len);
  int i;
  
  for (i = 0; i < ans_len; i++)
    NUM_SET(vec_elt(vr,i), NUM_GET(vec_elt(v1,start + i)));
  NPOP(2); VEC_PUSH(vr); 
}


/*Vector Comparison Operations.
If two vectors are of unequal length, the shorter one is padded with zeros.
i.e. The Comparison is lexicographical
Both LT and GT are defined to deal with infinities.
*/

int vlt_op(VEC_VAL *v0, VEC_VAL *v1) {
  int i;

  int n0 = v0->n;
  int n1 = v1->n;
  int nMax = MAX(n0,n1);
  
  for (i = 0; i < nMax; i++) {
    NUM_VAL x0 = (i < n0) ? num_vec_elt(v0,i) : 0;
    NUM_VAL x1 = (i < n1) ? num_vec_elt(v1,i) : 0;
    if (x0 < x1) {
      return 1;
    } else if (x0 > x1) {
      return 0;
    }    
  }
  return 0;
}

int vgt_op(VEC_VAL *v0, VEC_VAL *v1) {
  int i;

  int n0 = vec_len(v0);
  int n1 = vec_len(v1);
  int nMax = MAX(n0,n1);
  
  for (i = 0; i < nMax; i++) {
    NUM_VAL x0 = (i < n0) ? num_vec_elt(v0,i) : 0;
    NUM_VAL x1 = (i < n1) ? num_vec_elt(v1,i) : 0;
    if (x0 > x1) {
      return 1;
    } else if (x0 < x1) {
      return 0;
    }    
  }
  return 0;
}

int veq_op(VEC_VAL *v0, VEC_VAL *v1) {
  int i;

  int n0 = vec_len(v0);
  int n1 = vec_len(v1);
  int nMax = MAX(n0,n1);
  
  for (i = 0; i < nMax; i++) {
    NUM_VAL x0 = (i < n0) ? num_vec_elt(v0,i) : 0;
    NUM_VAL x1 = (i < n1) ? num_vec_elt(v1,i) : 0;
    if (x0 != x1) {
      return 0;
    }    
  }
  return 1;
}

void vlt_exec (VOID) {
  VEC_VAL *v0 = VEC_PEEK(1);
  VEC_VAL *v1 = VEC_PEEK(0);
  NPOP(2); NUM_PUSH(vlt_op(v0, v1));
}

void vgt_exec (VOID) {
  VEC_VAL *v0 = VEC_PEEK(1);
  VEC_VAL *v1 = VEC_PEEK(0);
  NPOP(2); NUM_PUSH(vgt_op(v0, v1));
}

void veq_exec (VOID) {
  VEC_VAL *v0 = VEC_PEEK(1);
  VEC_VAL *v1 = VEC_PEEK(0);
  NPOP(2); NUM_PUSH(veq_op(v0, v1));
}

void vlte_exec (VOID) {
  VEC_VAL *v0 = VEC_PEEK(1);
  VEC_VAL *v1 = VEC_PEEK(0);
  NPOP(2); NUM_PUSH(vlt_op(v0, v1) || veq_op(v0, v1));
}

void vgte_exec (VOID) {
  VEC_VAL *v0 = VEC_PEEK(1);
  VEC_VAL *v1 = VEC_PEEK(0);
  NPOP(2); NUM_PUSH(vgt_op(v0, v1) || veq_op(v0, v1));
}

void vmin_exec (VOID) {
  VEC_VAL *v0 = VEC_PEEK(1);
  VEC_VAL *v1 = VEC_PEEK(0);
  int is_less = vlt_op(v0, v1);
  NPOP(2); VEC_PUSH(is_less ? v0 : v1);
}

void vmax_exec (VOID) {
  VEC_VAL *v0 = VEC_PEEK(1);
  VEC_VAL *v1 = VEC_PEEK(0);
  int is_greater = vgt_op(v0, v1);
  NPOP(2); VEC_PUSH(is_greater ? v0 : v1);
}


//
// Feedback & Neighbor Operations
//

void init_feedback_exec (uint8_t state_off) {
  DATA res;
  // POST("M%d EXECING INIT-FEEDBACK %d %d\n", machine->id, machine->pc, state_off);
  if (!IS_OPEN(state_off)) {
    res = funcall0(FUN_PEEK(0));
    // POST("M%d RESETTING FEEDBACK %d -> ", machine->id, state_off);
    // post_data(&res); POST("\n");
    STATE_SET(state_off, &res);
    IS_OPEN_SET(state_off, 1);
  }
  NPOP(1); PUSH(STATE_GET(state_off));
}

void feedback_exec (uint8_t state_off) {
  DATA *res;
  // POST("M%d EXECING FEEDBACK %d %d\n", machine->id, machine->pc, state_off);
  IS_EXEC_SET(state_off, 1);
  res = PEEK(0);
  STATE_SET(state_off, res);
  NPOP(2); PUSH(res);
}

INLINE NUM_VAL nbr_bearing (VOID) {
  MACHINE *m = machine;
  return atan2f(m->nbr_x , m->nbr_y);
}

INLINE NUM_VAL nbr_lag (VOID) {
  MACHINE *m = machine;
  return m->nbr_lag;
}

INLINE NUM_VAL nbr_range (VOID) {
  MACHINE *m = machine;
  return sqrt(m->nbr_x*m->nbr_x + m->nbr_y*m->nbr_y + m->nbr_z*m->nbr_z);
}

// Approximates the area of the disc of radio communication about m.
// FIXME: What if we are in a three-dimensional space?

static NUM_VAL machine_radio_disc_area (MACHINE *m) {
  NUM_VAL radius = m->radio_range;
  return (radius * radius * M_PI);
}

#define NUM_AREA_BINS 16

NUM_VAL machine_area (MACHINE *m) {
  int i;
  /*  Deprecated for geometric incoherence
  // Note: this function assumes knowledge of neighbor relative X-Y position
  //   and operates only in 2D.  It should really be some smarter sort of 
  //   voronoi-ish calculation
  NUM_VAL radii[NUM_AREA_BINS];
  int counts[NUM_AREA_BINS];
  NUM_VAL radius, tot_radius, tot_count;
  for (i = 0; i < NUM_AREA_BINS; i++) {
    counts[i] = 0;
    radii[i]  = m->radio_range;
  }
  for (i = 0; i < m->n_hood; i++) {
    NBR *nbr = m->hood[i];
    flo angle = atan2f(nbr->y, nbr->x) + M_PI;
    flo range = sqrt(nbr->x*nbr->x + nbr->y*nbr->y); // once used 3D info badly
    int off = floor(NUM_AREA_BINS * angle / (2*M_PI));
    if (counts[off] == 0)
      radii[off] = 0;
    radii[off]  += range;
    counts[off] += 1;
  }
  tot_radius = tot_count = 0;
  for (i = 0; i < NUM_AREA_BINS; i++) {
    tot_radius += radii[i];
    tot_count  += MAX(1, counts[i]);
  }
  radius = (tot_radius / tot_count);
  */

  // This is an estimate made with no neighbor location information
  // given neighbor X/Y or X/Y/Z information, a better estimate can be made
  return (machine_radio_disc_area(m) / (m->n_hood + 1));
}

static NUM_VAL machine_density (MACHINE *m) {
  return ((m->n_hood + 1) / machine_radio_disc_area(m));
}

INLINE void fold_one (FUN_VAL fuse, uint16_t id, DATA *res, DATA *src) {
  DATA val = funcall2(fuse, res, src);
  if (is_debugging(machine)) { POST("M%d ", id); /* POST("[%.1f %.1f]", nbr_range(), nbr_bearing()); */ POST("<"); post_data(res); POST(" "); post_data(src); POST(">=>"); post_data(&val); POST(" "); }
  data_copy(res, &val);
}

void do_fold_hood_exec (DATA *res, int off) {
  MACHINE *m    = machine;
  FUN_VAL fuse  = FUN_PEEK(2);
  DATA *init    = PEEK(1);
  DATA *expr    = PEEK(0);
  int is_debug  = is_debugging(m);
  int i;
  //  char OutputMsg[32];
  data_copy(res, init);
  // if (off >= m->n_hood_vals) uerror("EXPORTS OVERFLOW");
  if (is_debug) { POST("M%d(%d,%d) ", m->id, m->n_hood, off); post_data(res); POST(" "); }
  m->nbr_x = m->nbr_y = m->nbr_z = 0;
  m->nbr_lag = m->time - m->last_time;
  fold_one(fuse, m->id, res, expr);
  DATA_SET(&m->hood_exports[off], expr);
  if (is_debug) { POST("{"); post_data(expr); POST("}"); }
  for (i = 0; i < m->n_hood; i++) {
    NBR *nbr = m->hood[i];
    DATA *import = &nbr->imports[off];
    if (!import->is_dead) {
      DATA imp;
      init_data(&imp, (TAG)expr->tag, import->val);
      m->nbr_x = nbr->x; m->nbr_y = nbr->y; m->nbr_z = nbr->z; 
      m->nbr_lag = m->time - nbr->time;
      fold_one(fuse, m->hood[i]->id, res, &imp);
    } else if (is_debug)
      POST("M%d DEAD ", m->hood[i]->id);
  }    
  if (is_debug) { POST("=> "); post_data(res); POST("\n"); }
  NPOP(3); PUSH(res);
}

INLINE void mold_fold_one (FUN_VAL mold, FUN_VAL fuse, uint16_t id, DATA *res, DATA *src) {
  DATA x   = funcall1(mold, src);
  DATA val = funcall2(fuse, res, &x);
  if (is_debugging(machine)) { POST("M%d ", id); /* POST("[%.1f %.1f]", nbr_range(), nbr_bearing()); */ POST("<"); post_data(res); POST(" "); post_data(&x); POST(">=>"); post_data(&val); POST(" "); }
  data_copy(res, &val);
}

void do_fold_hood_plus_exec (DATA *res, int off) {
  MACHINE *m    = machine;
  FUN_VAL fuse  = FUN_PEEK(2);
  FUN_VAL mold  = FUN_PEEK(1);
  DATA *expr    = PEEK(0);
  DATA val;
  int is_debug  = is_debugging(m);
  int i;
  //  char OutputMsg[32];
  // if (off >= m->n_hood_vals) uerror("EXPORTS OVERFLOW");
  if (is_debug) { POST("M%d(%d,%d) ", m->id, m->n_hood, off); post_data(res); POST(" "); }
  m->nbr_x = m->nbr_y = m->nbr_z = 0;
  m->nbr_lag = m->time - m->last_time;
  val = funcall1(mold, expr);
  data_copy(res, &val);
  DATA_SET(&m->hood_exports[off], expr);
  if (is_debug) { POST("{"); post_data(expr); POST("}"); }
  for (i = 0; i < m->n_hood; i++) {
    NBR *nbr = m->hood[i];
    DATA *import = &nbr->imports[off];
    if (!import->is_dead) {
      DATA imp;
      init_data(&imp, (TAG)expr->tag, import->val);
      m->nbr_x = nbr->x; m->nbr_y = nbr->y; m->nbr_z = nbr->z; 
      m->nbr_lag = m->time - nbr->time;
      mold_fold_one(mold, fuse, m->hood[i]->id, res, &imp);
    } else if (is_debug)
      POST("M%d DEAD ", m->hood[i]->id);
  }    
  if (is_debug) { POST("=> "); post_data(res); POST("\n"); }
  NPOP(3); PUSH(res);
}

void apply_exec (VOID) {
  FUN_VAL  fun  = FUN_PEEK(1);
  VEC_VAL *args = VEC_PEEK(0);
  DATA res;
  int n = vec_len(args);
  int i;

  FUN_PUSH(machine->pc);
  for (i = 0; i < n; i++) 
    ENV_PUSH(vec_elt(args, i));
  eval(&res, fun);
  ENV_POP(n);
  NPOP(2); PUSH(&res);
}

void map_exec (int off) {
  VEC_VAL *res = VEC_GET(GLO_GET(off));
  FUN_VAL  fun = FUN_PEEK(1);
  VEC_VAL *vec = VEC_PEEK(0);
  int n        = vec_len(vec);
  int i;
  vec_zap(res);
  for (i = 0; i < n; i++) {
    DATA val = funcall1(fun, vec_elt(vec, i));
    res = vec_add(res, &val);
  }
  VEC_SET(GLO_GET(off), res);
  NPOP(2); VEC_PUSH(res);
}

void do_fold_exec (DATA *res) {
  FUN_VAL  fun = FUN_PEEK(2);
  DATA   *init = PEEK(1);
  VEC_VAL *vec = VEC_PEEK(0);
  int n        = vec_len(vec);
  int i;
  data_copy(res, init);
  for (i = 0; i < n; i++) {
    DATA val = funcall2(fun, res, vec_elt(vec, i));
    data_copy(res, &val);
  }
  NPOP(3); PUSH(res);
}

void fold_exec (VOID) {
  DATA res; do_fold_exec(&res);
}

void vfold_exec (int roff) {
  DATA *res = GLO_GET(roff);
  do_fold_exec(res);
}

static int max_stack_size = 0;

INLINE void check_stack (MACHINE *m) {
  int delta = (m->sp - m->stack);
  max_stack_size = MAX(delta, max_stack_size);
  if (delta > m->n_stack)
    uerror("STACK OVERFLOW %d > %d\n", delta, m->n_stack);
  if (delta < 0)
    uerror("STACK UNDERFLOW\n");
}

static int max_env_size = 0;

INLINE void check_env (MACHINE *m) {
  int delta = (m->ep - m->env);
  max_env_size = MAX(delta, max_env_size);
  if (delta > m->n_env)
    uerror("ENV OVERFLOW %d > %d\n", delta, m->n_env);
  if (delta < 0)
    uerror("ENV UNDERFLOW\n");
}

INLINE void dump_stack (MACHINE *m) {
   ///@TODO: add a way to call delft_dump_stack conditionally
   if (is_tracing(m)) {
      proto_dump_stack(m);
   }
}

/** 
 * Original PROTO-style stack dump: default
 */
void proto_dump_stack (MACHINE *m) {
   int i;
   int max_dump = 6; // 99
   POST("---\n");
   // POST("S[%d]E[%d]\n", max_stack_size, max_env_size);
   for (i = 0; i < MIN(m->sp - m->stack, max_dump); i++) {
      POST("->S[%d] ", i); post_data(PEEK(i)); POST("\n");
   }
   for (i = 0; i < MIN(m->ep - m->env, max_dump); i++) {
      POST("->E[%d] ", i); post_data(ENV_PEEK(i)); POST("\n");
   }
}

/** 
 * PROTO-delft-style stack dump
 */
void delft_dump_stack (MACHINE *m) {
   int i;
   int max_dump = 6; // 99
   if( MAX(m->sp - m->stack, m->ep - m->env) == 0 )
      return;
   POST("-------------------\n");
   POST("pos \t stack \t env\n");
   POST("--- \t ----- \t ---\n");
   for( i=MAX(MAX(m->sp - m->stack, m->ep - m->env),max_dump)-1; i >= 0; i-- ) {
      POST( " %d \t ", i);
      if(m->sp - m->stack >= i) 
         post_data(PEEK(i));
      POST("  \t ");
      if(m->ep - m->env   >= i) 
         post_data(ENV_PEEK(i));
      POST("  \t ");
      POST("\n");
   }
   POST("-------------------\n");
}

typedef union {
  flo val;
  uint8_t bytes[4];
  uint32_t fourBytes;
} FLO_BYTES;

MAYBE_INLINE void def_fun_exec (MACHINE *m, uint16_t skip) {
  DATA res;
  FUN_SET(&res, m->pc);
  m->pc += skip;
  GLO_SET(m->n_globals++, &res);
}

/**
 * Puts n elements on the stack into the environment.
 */
void let_exec(MACHINE* m, int n) {
   uint8_t i;
   // if (is_trace) { POST("BEF LET N %d SP %lx ST %lx DELTA %d\n", n, m->sp, m->stack, (m->sp - m->stack)); }
   for (i = n; i > 0; i--)
      DATA_SET(m->ep++, (m->sp-i));
   // if (is_trace) { POST("MID LET N %d EDELTA %d SP %lx ST %lx DELTA %d\n", n, (m->ep - m->env), m->sp, m->stack, (m->sp - m->stack)); }
   m->sp -= n;
   // if (is_trace) { POST("AFT LET N %d SP %lx ST %lx DELTA %d\n", n, m->sp, m->stack, (m->sp - m->stack)); }
}

/**
 * Remove the last n elements from the environment.
 */
void clean_env(MACHINE* m, int n) {
   m->ep -= n;
}


#define INF 1.0e12

DATA *eval(DATA *res, FUN_VAL fun) {
  MACHINE *m     = machine;
  SCRIPT *script = &m->scripts[m->cur_script];
  uint8_t op, i;
  int is_trace = is_tracing(m);
  // char msg[16];
  m->pc = fun;

  if (fun > script->len) 
    uerror("BOGUS FUNCTION %d\n", fun);
  if (is_trace)
    POST("M%d EVAL AT %d\n", m->id, fun);
  dump_stack(m);
  for (i = 0;;i++) {
    // putstring(" PC:"); putnum_ud(m->pc); 
    op = NXT_OP(m);
    if (is_trace) {
      const char *name = CORE_OPCODES_STR[op];
      POST("%3d: OP %3d %s\n", m->pc-1, op, name); 
    }
    if(op < CORE_CMD_OPS) {
      switch (op) {
      case DEF_VM_OP: {
   int export_len  = NXT_OP(m);
   int n_exports   = NXT_OP(m);
   int n_globals   = NXT_OP16(m);
   int n_state     = NXT_OP(m);
   int n_stack     = NXT_OP16(m);
   int n_env       = NXT_OP(m);
   if (is_debugging(m))
      POST("M%d MEM SIZE %d\n", m->id, machine_mem_size(m));
   if (is_debugging(m)) 
      POST("EXPORT_LEN %d N_EXPORTS %d N_GLOBALS %d N_STATE %d N_STK %d N_ENV %d\n",
           export_len, n_exports, n_globals, n_state, n_stack, n_env);
   m->n_hood_vals = n_exports;
   m->max_globals = n_globals;
   m->n_state = n_state;
   // m->n_stack = n_stack;
   m->hood_exports = (DATA*)PMALLOC(n_exports * sizeof(DATA));
   m->buflen = export_len * sizeof(DATA);
   m->buf = (COM_DATA*)PMALLOC(m->buflen);
   memset(m->buf,      0, m->buflen);
   if (is_throttling) {
      m->next_buf = (COM_DATA*)PMALLOC(m->buflen);
      memset(m->next_buf, 0, m->buflen);
   } else
      m->next_buf = m->buf;
   if (are_exports_serialized) {
      for (i = 0; i < MAX_HOOD; i++)
         m->hood_data[i].imports = (DATA*)PMALLOC(n_exports * sizeof(DATA));
   }
   m->state = (STATE*)PMALLOC(n_state * sizeof(STATE));
   m->globals = (DATA*)PMALLOC(n_globals * sizeof(DATA));
   m->n_stack = n_stack;
   m->stack = (DATA*)PMALLOC(n_stack * sizeof(DATA));
   m->n_env = n_env;
   m->env = (DATA*)PMALLOC(n_env * sizeof(DATA));
   if (is_debugging(m))
      POST("M%d MEM SIZE %d\n", m->id, machine_mem_size(m));
   m->sp = m->stack;
   m->ep = m->env;
   // m->n_env = n_env;
   break; 
         }
      case EXIT_OP: 
   return res; 
      case INF_OP:
   NUM_PUSH(INFINITY); break; 
      case LIT8_OP: 
      case LIT_0_OP: case LIT_1_OP: case LIT_2_OP: case LIT_3_OP: case LIT_4_OP: {
  int n = op == LIT8_OP ? NXT_OP(m) : op - LIT_0_OP;
  NUM_PUSH(n);
  break; }
      case LIT16_OP: {
   NUM_VAL val = NXT_OP16(m);
   NUM_PUSH(val);
   break; }
      case LIT_FLO_OP: {
   FLO_BYTES fb;
   fb.bytes[3] = NXT_OP(m); //
   fb.bytes[2] = NXT_OP(m); // Pack th' float in network
   fb.bytes[1] = NXT_OP(m); // byte order (big endian).
   fb.bytes[0] = NXT_OP(m); //
   fb.fourBytes = ntohl(fb.fourBytes); // Convert float into host byte order.
   NUM_PUSH(fb.val); // Load onto stack in float form.
   break; }
      case DEF_OP: {
   DATA data;
   GLO_SET(m->n_globals++, POP(&data));
   break; }
      case DEF_TUP_OP: case FAB_TUP_OP: {
   VEC_VAL *v;
   DATA data, num;
   int n = NXT_OP(m);
   init_vec(&data, n, n, init_num(&num, 0.0));
   v = VEC_GET(&data); 
   for (i = 0; i < n; i++)
     DATA_SET(&v->elts[i], PEEK(n-i-1));
   NPOP(n); 
   if (op == DEF_TUP_OP) 
     GLO_SET(m->n_globals++, &data);
   else
     PUSH(&data);	
   break; }
      case NUL_TUP_OP: 
   PUSH(&nul_tup_dat);
   break;
      case DEF_NUM_VEC_OP: case DEF_NUM_VEC_1_OP: case DEF_NUM_VEC_2_OP: case DEF_NUM_VEC_3_OP:
      case FAB_NUM_VEC_OP: {
   DATA data, num;
   int n = (op == DEF_NUM_VEC_OP || op == FAB_NUM_VEC_OP) ? NXT_OP(m) : op - DEF_NUM_VEC_OP;
   init_vec(&data, n, n, init_num(&num, 0.0));
   if (op != FAB_NUM_VEC_OP) 
     GLO_SET(m->n_globals++, &data);
   else
     PUSH(&data);	
   break; }
      case DEF_VEC_OP: case FAB_VEC_OP: {
   VEC_VAL *v;
   DATA data, arg, num;
   int n = NXT_OP(m);
   init_vec(&data, n, n, init_num(&num, 0.0));
   v = VEC_GET(&data); 
   POP(&arg);
   for (i = 1; i < n; i++)
     data_copy(&v->elts[i], &arg);
   if (op == DEF_VEC_OP) 
     GLO_SET(m->n_globals++, &data);
   else
     PUSH(&data);	
   break; }
      case LET_OP: 
      case LET_1_OP: case LET_2_OP: case LET_3_OP: case LET_4_OP: {
   int n = op == LET_OP ? NXT_OP(m) : op - LET_OP;
   let_exec(m, n);
   break; }
      case REF_OP: 
      case REF_0_OP: case REF_1_OP: case REF_2_OP: case REF_3_OP: {
   int off = op == REF_OP ? NXT_OP(m) : op - REF_0_OP;
   PUSH(m->ep-off-1);
   break; }
      case GLO_REF16_OP: {
   uint16_t off =  NXT_OP16(m);
   PUSH(&m->globals[off]);
   break; }
      case GLO_REF_OP: 
      case GLO_REF_0_OP: case GLO_REF_1_OP: case GLO_REF_2_OP: case GLO_REF_3_OP: {
   int off = op == GLO_REF_OP ? NXT_OP(m) : op - GLO_REF_0_OP;
   PUSH(&m->globals[off]);
   break; }
      case POP_LET_OP: 
      case POP_LET_1_OP: case POP_LET_2_OP: case POP_LET_3_OP: case POP_LET_4_OP: {
   int n = op == POP_LET_OP ? NXT_OP(m) : op - POP_LET_OP;
   m->ep -= n;
   break; }
      case NO_OP: 
   break;
      case MUX_OP: {
   DATA *reso = NUM_PEEK(2) ? PEEK(1) : PEEK(0); NPOP(3); PUSH(reso);
   break; }
      case VMUX_OP: {
   int roff = NXT_OP(m);
   DATA *res = GLO_GET(roff);
   if (NUM_PEEK(2))
     data_copy(res, PEEK(1));
   else 
     data_copy(res, PEEK(0)); 
   NPOP(3); PUSH(res);
   break; }
      case IF_16_OP: {
   uint16_t else_off = NXT_OP16(m);
   NUM_VAL tst  = NUM_POP();
   if (tst != 0)
     m->pc += else_off;
   break; }
      case IF_OP: {
   int else_off = NXT_OP(m);
   NUM_VAL tst  = NUM_POP();
   if (tst != 0)
     m->pc += else_off;
   break; }
      case JMP_OP: {
   int off = NXT_OP(m);
   //POST("JUMPING FROM %d TO %d \n", m->pc, m->pc+off);
   m->pc += off; break; }
      case JMP_16_OP: {
   int off = NXT_OP16(m);
   m->pc += off; break; }
      case RET_OP: {
   POP(res);
   m->pc = FUN_POP();
   dump_stack(m);
   return res; }
      case DEF_FUN_2_OP: case DEF_FUN_3_OP: case DEF_FUN_4_OP: 
      case DEF_FUN_5_OP: case DEF_FUN_6_OP: case DEF_FUN_7_OP:
   def_fun_exec(m, op - DEF_FUN_2_OP + 2); break;
      case DEF_FUN_OP: 
   def_fun_exec(m, NXT_OP(m)); break;
      case DEF_FUN16_OP: 
   def_fun_exec(m, NXT_OP16(m)); break;
      case SET_DT_OP:
   set_dt(NUM_PEEK(0)); break;
      case DT_OP: 
   NUM_PUSH(machine->time==0 ? machine->desired_period : machine->time - machine->last_time); break;
      case PROBE_OP: {
   DATA *val = PEEK(1);
   int k     = (int)NUM_PEEK(0);
   set_probe(val, k);
   NPOP(2); PUSH(val);
   break; }
      case INFINITESIMAL_OP: 
   NUM_PUSH(machine_area(m)); break;
      case DENSITY_OP: 
   NUM_PUSH(machine_density(m)); break;
      case NBR_RANGE_OP: 
   NUM_PUSH(nbr_range()); break;
      case AREA_OP: 
   NUM_PUSH(machine_area(m)); break;
      case NBR_BEARING_OP: 
   NUM_PUSH(nbr_bearing()); break;
      case NBR_LAG_OP: 
   NUM_PUSH(nbr_lag()); break;
      case NBR_VEC_OP: {
   VEC_VAL *vr = VEC_GET(GLO_GET(NXT_OP(m)));
   ensure_len(vr, 3);
   NUM_SET(vec_elt(vr, 0), m->nbr_x);
   NUM_SET(vec_elt(vr, 1), m->nbr_y);
   NUM_SET(vec_elt(vr, 2), m->nbr_z);
   VEC_PUSH(vr);
   break; }
      case BEARING_OP: 
   NUM_PUSH(read_bearing()); break;
      case SPEED_OP: 
   NUM_PUSH(read_speed()); break;
      case MOV_OP: 
   mov(VEC_PEEK(0)); break;
      case RND_OP: { 
   NUM_VAL val = num_rnd(NUM_PEEK(1), NUM_PEEK(0)); 
   NPOP(2); NUM_PUSH(val); 
   break; }
   //Scalar Comparison Operations
      case GT_OP: {
   NUM_VAL val = NUM_PEEK(1) > NUM_PEEK(0); NPOP(2); NUM_PUSH(val); break; }
      case GTE_OP: {
   NUM_VAL val = NUM_PEEK(1) >= NUM_PEEK(0); NPOP(2); NUM_PUSH(val); break; }
      case LT_OP: { 
   NUM_VAL val = NUM_PEEK(1) < NUM_PEEK(0); NPOP(2); NUM_PUSH(val); break; }
      case LTE_OP: {
   NUM_VAL val = NUM_PEEK(1) <= NUM_PEEK(0); NPOP(2); NUM_PUSH(val); break; }
      case EQ_OP: {
   NUM_VAL val = NUM_PEEK(1) == NUM_PEEK(0); NPOP(2); NUM_PUSH(val); break; }
      case MAX_OP: {
   NUM_VAL val = MAX(NUM_PEEK(1), NUM_PEEK(0)); NPOP(2); NUM_PUSH(val); break; }
      case MIN_OP: {
   NUM_VAL val = MIN(NUM_PEEK(1), NUM_PEEK(0)); NPOP(2); NUM_PUSH(val); break; }
   //Basic Mathematical Operations
      case NOT_OP: {
   NUM_VAL val = (0 == NUM_PEEK(0)); NPOP(1); NUM_PUSH(val); break; }
      case ADD_OP: {
   NUM_VAL val = NUM_PEEK(1) + NUM_PEEK(0); NPOP(2); NUM_PUSH(val); break; }
      case MOD_OP: {
        //TODO: this is also implemented in compiler/analyzer.cpp,
        //      merge these implementations!
        NUM_VAL dividend = NUM_PEEK(1);
        NUM_VAL divisor = NUM_PEEK(0); 
        NUM_VAL val = fmod(fabs(dividend), fabs(divisor)); 
        if(divisor < 0 && dividend < 0) val *= -1;
        else if(dividend < 0) val = divisor - val;
        else if(divisor < 0) val += divisor;
          NPOP(2); NUM_PUSH(val); break; 
      }
      case REM_OP: {
   NUM_VAL val = fmod(NUM_PEEK(1), NUM_PEEK(0)); NPOP(2); NUM_PUSH(val); break; }
      case DIV_OP: {
   NUM_VAL val = NUM_PEEK(1) / NUM_PEEK(0); NPOP(2); NUM_PUSH(val); break; }
      case MUL_OP: {
   NUM_VAL val = NUM_PEEK(1) * NUM_PEEK(0); NPOP(2); NUM_PUSH(val); break; }
      case SUB_OP: {
   NUM_VAL val = NUM_PEEK(1) - NUM_PEEK(0); NPOP(2); NUM_PUSH(val); break; }
   //Math Operations
      case ABS_OP: 
   NUM_PUSH(fabs(NUM_POP()));  break; 
      case POW_OP: {
   NUM_VAL val = pow(NUM_PEEK(1), NUM_PEEK(0)); NPOP(2); NUM_PUSH(val); break; }
      case LOG_OP: {
   NUM_PUSH(log(NUM_POP())); break; }
      case SQRT_OP: {
   NUM_PUSH(sqrt(NUM_POP())); break; }
      case SIN_OP: {
   NUM_PUSH(sin(NUM_POP())); break; }
      case COS_OP: {
   NUM_PUSH(cos(NUM_POP())); break; }
      case TAN_OP: {
   NUM_PUSH(tan(NUM_POP())); break; }
      case SINH_OP: {
   NUM_PUSH(sinh(NUM_POP())); break; }
      case COSH_OP: {
   NUM_PUSH(cosh(NUM_POP())); break; }
      case TANH_OP: {
   NUM_PUSH(tanh(NUM_POP())); break; }      
      case ASIN_OP: {
   NUM_PUSH(asin(NUM_POP())); break; }      
      case ACOS_OP: {
   NUM_PUSH(acos(NUM_POP())); break; }      
      case FLOOR_OP: {
   NUM_PUSH(floor(NUM_POP())); break; }      
      case CEIL_OP: {
   NUM_PUSH(ceil(NUM_POP())); break; }            
      case ROUND_OP: {
   NUM_PUSH(rint(NUM_POP())); break; }
      case ATAN2_OP: {
   NUM_VAL val = atan2(NUM_PEEK(1), NUM_PEEK(0)); NPOP(2); NUM_PUSH(val); break; }      
   
   //Basic Vector Operations
      case VADD_OP: { 
   vadd_exec(NXT_OP(m)); break; }
      case VSUB_OP: {
   vsub_exec(NXT_OP(m)); break; }
      case VMUL_OP: {
   vmul_exec(NXT_OP(m)); break; }
      case VDOT_OP: {
   vdot_exec(); break; }
      case LEN_OP: {
   len_exec(); break; }
      case VSLICE_OP: {
   vslice_exec(NXT_OP(m)); break; }
   
   //Vector Comparison Operators
      case VGT_OP: {
   vgt_exec(); break; }
      case VGTE_OP: {
   vgte_exec(); break; }
      case VLT_OP: {
   vlt_exec(); break; }
      case VLTE_OP: {
   vlte_exec(); break; }
      case VEQ_OP: {
   veq_exec(); break; }
      case VMIN_OP: { 
   vmin_exec(); break; }
      case VMAX_OP: {
   vmax_exec(); break; }
   //Feedback and Neighbor Operations
      case INIT_FEEDBACK_OP: init_feedback_exec(NXT_OP(m)); break;
      case FEEDBACK_OP: feedback_exec(NXT_OP(m)); break;
      case FOLD_HOOD_OP: {
   DATA reso; do_fold_hood_exec(&reso, NXT_OP(m)); break; }
      case VFOLD_HOOD_OP: {
   int roff = NXT_OP(m);
   int off = NXT_OP(m);
   do_fold_hood_exec(GLO_GET(roff), off); break; }
      case FOLD_HOOD_PLUS_OP: {
   DATA reso; do_fold_hood_plus_exec(&reso, NXT_OP(m)); break; }
      case VFOLD_HOOD_PLUS_OP: {
   int roff = NXT_OP(m);
   int off  = NXT_OP(m);
   do_fold_hood_plus_exec(GLO_GET(roff), off); break; }
      case APPLY_OP: 
   apply_exec(/* NXT_OP(m) */); break;
      case ALL_OP: {
   int n = NXT_OP(m); DATA val; POP(&val); NPOP(n-1); PUSH(&val); break; }
   
      case FOLD_OP: 
   fold_exec(); break;
      case VFOLD_OP: 
   vfold_exec(NXT_OP(m)); break;
      case MAP_OP: 
   map_exec(NXT_OP(m)); break;
      case HOOD_RADIUS_OP: 
   NUM_PUSH(read_radio_range()); break;
      case TUP_OP: {
   int off = NXT_OP(m);
   int n   = NXT_OP(m);
   tup_exec(n, off); break; }
      case ELT_OP: 
   elt_exec(); break;
      case MID_OP:
   NUM_PUSH(m->id); break;
      case FLEX_OP:
   flex(NUM_PEEK(0)); break;
      case FUNCALL_OP: case FUNCALL_0_OP: case FUNCALL_1_OP: 
      case FUNCALL_2_OP: case FUNCALL_3_OP: case FUNCALL_4_OP: 
   {
      //n = number of arguments
      int n = (op == FUNCALL_OP) ? NXT_OP(m) : op - FUNCALL_0_OP;
      //pop function off stack
      FUN_VAL fun = FUN_GET(POP(res));
      //pop arguments, put them in env
      let_exec(m, n); //let n where n=num args
      //exec the function
      DATA ret = funcall0(fun);
      //remove n arguments from the env
      clean_env(m, n);
      //push ret onto stack
      PUSH(&ret);
      break;
   }
      default:
   uerror("UNKNOWN OPCODE %d %d\n", op, CORE_CMD_OPS);
      }
    } else {
      platform_operation(op); // call out to platform-specific code
    }
    dump_stack(m);
    check_stack(m);
    check_env(m);
  }
}

void clear_pkt_tracker(){
  MACHINE *m = machine;
  size_t i;
  for(i = 0; i< NUM_SCRIPT_PKTS + 1; i++){
    ATOMIC {
      m->pkt_tracker[i] = m->pkt_listner[i] = 0;
    }
  }
}


//ret: 0-already have pkt, 1-added pkt
uint8_t add_pkt(uint8_t pkt_num){
  MACHINE *m = machine;
  uint8_t  major, minor;
  major = (int)floor((float)pkt_num / (float)8);
  minor = pkt_num % 8;

  if (((uint8_t)1 << minor) & m->pkt_tracker[major])
    return 0;
  m->pkt_tracker[major] = (1 << minor) | m->pkt_tracker[major];
  return 1;
}

// true when the machine needs to request script or fill a neighbor's request
int script_export_needed(VOID) {
  MACHINE *m = machine;
  if(!m->scripts[m->cur_script].is_complete) return 1;
  size_t i;
  SCRIPT *nxt_script;
  // code ripped from export_script
  if (m->scripts[(m->cur_script+1)%MAX_SCRIPTS].version > m->scripts[m->cur_script].version)
    nxt_script = &m->scripts[(m->cur_script+1)%MAX_SCRIPTS];
  else
    nxt_script = &m->scripts[m->cur_script];
  for (i = 0; i < num_pkts(nxt_script->len); i++) {
    uint8_t major = i / 8;
    uint8_t minor = i % 8;
    if (m->pkt_listner[major] & (1 << minor)) {
    	return 1;
    }
  }
  return 0;
}

int is_script_complete(VOID) {
  MACHINE *m = machine;
  SCRIPT *script = &m->scripts[(m->cur_script+1)%MAX_SCRIPTS];
  uint8_t npkts = num_pkts(script->len);
  size_t i;
  uint8_t major, minor;

  major = npkts / 8;
  minor = npkts % 8;
  for (i = 0; i < major; i++) {
    if (is_script_debugging(m))
      POST("MAJOR %d %x\n", i, m->pkt_tracker[i]);
    if (m->pkt_tracker[i] != 0xff)
      return 0;
  }
  if (is_script_debugging(m))
    POST("MINOR %x\n", m->pkt_tracker[major]);
  if (minor > 0 && m->pkt_tracker[major] != ((1 << minor) - 1)) 
    return 0;
  return 1;
}

void install_script_pkt(MACHINE *m, uint8_t pkt_num, uint8_t *script_pkt, uint16_t len) {
  SCRIPT *script = &m->scripts[(m->cur_script+1)%MAX_SCRIPTS];
  uint16_t pkt_len = MIN(len - pkt_num * MAX_SCRIPT_PKT, MAX_SCRIPT_PKT);

  // dump_code(len, script_pkt);
  memcpy(&script->bytes[pkt_num * MAX_SCRIPT_PKT], script_pkt, pkt_len);
  script->len = len;
  add_pkt(pkt_num);
}

void link_script (MACHINE *m) {
  DATA res;
  SCRIPT *script = &m->scripts[m->cur_script];

  if (is_script_debugging(m))
    POST("M%d LINKING SCRIPT\n", m->id);
  m->script = script->bytes;
  m->n_state = 0;
  m->n_globals = 0;
  m->max_globals = 0;
  m->n_hood_vals = 0;
  m->sp = m->stack = NULL;
  m->ep = m->env   = NULL;
  reinit_machine(m);
  // dump_code(script->len, script->bytes);
  eval(&res, 0);
  if (is_tracing(m))
    POST("M%d DONE LINKING\n", m->id);
  NUM_POP();
  m->exec = FUN_GET(GLO_GET(m->n_globals-1));
}

void install_script(MACHINE *m, uint8_t *script, uint8_t slot, uint16_t len, uint8_t version) {
  SCRIPT *s = &m->scripts[slot];
  size_t i;
  if (len > MAX_SCRIPT_LEN)
    uerror("SCRIPT TOO BIG %d > %d -- INCREASE MAX_SCRIPT_LEN\n", 
      len, MAX_SCRIPT_LEN);
  // post("SIZEOF DATA %d\n", sizeof(DATA));
  memcpy(s->bytes, script, len);
  s->is_complete = TRUE;
  s->len = len;
  s->version = version;
  clear_pkt_tracker();
  for(i = 0; i < num_pkts(len); i++)
    add_pkt(i);
  link_script(m);
}
