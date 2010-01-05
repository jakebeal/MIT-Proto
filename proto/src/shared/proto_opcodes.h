/* ProtoKernel opcodes; also version info
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __PROTO_OPCODES__
#define __PROTO_OPCODES__

#ifdef __cplusplus
extern "C" {
#endif

#define PROTO_VERSION 402

#define MAX_GLO_REF_OPS     4
#define MAX_REF_OPS         4
#define MAX_LET_OPS         4
#define MAX_DEF_FUN_OPS     6
#define MAX_DEF_NUM_VEC_OPS 3
#define MAX_LIT_OPS         5

typedef enum {
  // Low-level opcodes
  RET_OP,
  EXIT_OP,
  LIT8_OP,
  LIT_0_OP,
  LIT_1_OP,
  LIT_2_OP,
  LIT_3_OP,
  LIT_4_OP,
  LIT16_OP,
  LIT_FLO_OP,
  NUL_TUP_OP,
  FAB_TUP_OP,
  DEF_TUP_OP,
  FAB_VEC_OP,
  DEF_VEC_OP,
  FAB_NUM_VEC_OP,
  DEF_NUM_VEC_OP,
  DEF_NUM_VEC_1_OP,
  DEF_NUM_VEC_2_OP,
  DEF_NUM_VEC_3_OP,
  DEF_OP,
  REF_OP,
  REF_0_OP,
  REF_1_OP,
  REF_2_OP,
  REF_3_OP,
  DEF_VM_OP,
  GLO_REF16_OP,
  GLO_REF_OP,
  GLO_REF_0_OP,
  GLO_REF_1_OP,
  GLO_REF_2_OP,
  GLO_REF_3_OP,
  LET_OP,
  LET_1_OP,
  LET_2_OP,
  LET_3_OP,
  LET_4_OP,
  POP_LET_OP,
  POP_LET_1_OP,
  POP_LET_2_OP,
  POP_LET_3_OP,
  POP_LET_4_OP,
  // Universal sensing & actuation ops
  SET_DT_OP,
  MOV_OP,
  PROBE_OP,
  HOOD_RADIUS_OP,
  AREA_OP,
  FLEX_OP,
  INFINITESIMAL_OP,
  DT_OP,
  NBR_RANGE_OP,
  NBR_BEARING_OP,
  NBR_VEC_OP,
  NBR_LAG_OP,
  MID_OP,
  SPEED_OP,
  BEARING_OP,
  // Math Opcodes
  INF_OP,
  ELT_OP,
  RND_OP,
  ADD_OP,
  SUB_OP,
  MUL_OP,
  DIV_OP,
  FLOOR_OP,
  CEIL_OP,
  MOD_OP,
  POW_OP,
  LOG_OP,
  SQRT_OP,
  SIN_OP,
  COS_OP,
  TAN_OP,
  SINH_OP,
  COSH_OP,
  TANH_OP,
  ASIN_OP,
  ACOS_OP,
  ATAN2_OP,
  LT_OP,
  LTE_OP,
  GT_OP,
  GTE_OP,
  EQ_OP,
  MAX_OP,
  MIN_OP,
  ABS_OP,
  //Vector & Tuple Opcodes
  TUP_OP,
  VEC_OP,
  LEN_OP,
  VFIL_OP,
  VADD_OP,
  VDOT_OP,
  VMUL_OP,
  VSUB_OP,
  VSLICE_OP,
  VLT_OP,
  VLTE_OP,
  VGT_OP,
  VGTE_OP,
  VEQ_OP,
  VMIN_OP,
  VMAX_OP,   
  //Special Forms Opcodes
  APPLY_OP,
  MAP_OP,
  FOLD_OP,
  VFOLD_OP,
  DEF_FUN_2_OP,
  DEF_FUN_3_OP,
  DEF_FUN_4_OP,
  DEF_FUN_5_OP,
  DEF_FUN_6_OP,
  DEF_FUN_7_OP,
  DEF_FUN_OP,
  DEF_FUN16_OP,
  FOLD_HOOD_OP,
  VFOLD_HOOD_OP,
  FOLD_HOOD_PLUS_OP,
  VFOLD_HOOD_PLUS_OP,
  INIT_FEEDBACK_OP,
  FEEDBACK_OP,
  // Control flow opcodes
  ALL_OP,
  NO_OP,
  MUX_OP,
  VMUX_OP,
  IF_OP,
  IF_16_OP,
  JMP_OP,
  JMP_16_OP,
  CORE_CMD_OPS,
} CORE_OPCODES;

typedef uint8_t OPCODE;

#ifdef __cplusplus
}
#endif
#endif


