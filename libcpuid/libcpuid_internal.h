/*
 * Copyright 2016  Veselin Georgiev,
 * anrieffNOSPAM @ mgail_DOT.com (convert to gmail)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __LIBCPUID_INTERNAL_H__
#define __LIBCPUID_INTERNAL_H__
/*
 * This file contains internal undocumented declarations and function prototypes
 * for the workings of the internal library infrastructure.
 */

#define EXTRACTS_BIT(reg, bit)              ((reg >> bit)    & 0x1)
#define EXTRACTS_BITS(reg, highbit, lowbit) ((reg >> lowbit) & ((1ULL << (highbit - lowbit + 1)) - 1))

enum _common_codes_t {
	NA = 0,
	NC, /* No code */
};

enum _cache_type_t {
	L1I,
	L1D,
	L2,
	L3,
	L4,
	NUM_CACHE_TYPES
};
typedef enum _cache_type_t cache_type_t;

#define CODE(x) x
#define CODE2(x, y) x = y
enum _amd_code_t {
	#include "amd_code_t.h"
};
typedef enum _amd_code_t amd_code_t;

enum _centaur_code_t {
	#include "centaur_code_t.h"
};
typedef enum _centaur_code_t centaur_code_t;

enum _intel_code_t {
	#include "intel_code_t.h"
};
typedef enum _intel_code_t intel_code_t;
#undef CODE
#undef CODE2

struct internal_id_info_t {
	union {
		amd_code_t     amd;
		centaur_code_t centaur;
		intel_code_t   intel;
	} code;
	uint64_t bits;
	int score; // detection (matchtable) score
	int32_t cache_mask[NUM_CACHE_TYPES];
};

struct internal_topology_t {
	int32_t apic_id;
	int32_t package_id;
	int32_t core_id;
	int32_t smt_id;
	int32_t cache_id[NUM_CACHE_TYPES];
	logical_cpu_t logical_cpu;
};

struct internal_core_id_t {
	logical_cpu_t num_logical_cpu;
	int32_t core_id;
};

struct internal_cache_id_t {
	logical_cpu_t num_logical_cpu;
	int32_t cache_id;
};

#define CORES_HTABLE_SIZE 256
struct internal_core_instances_t {
	uint8_t instances;
	struct internal_core_id_t htable[CORES_HTABLE_SIZE];
};

#define CACHES_HTABLE_SIZE 256
struct internal_cache_instances_t {
	uint8_t instances[NUM_CACHE_TYPES];
	struct internal_cache_id_t htable[NUM_CACHE_TYPES][CACHES_HTABLE_SIZE];
};

struct internal_type_info_t {
	cpu_purpose_t purpose;
	int32_t package_id;
	struct internal_id_info_t id_info;
	struct internal_core_instances_t core_instances;
	struct internal_cache_instances_t cache_instances;
};

struct internal_type_info_array_t {
	uint8_t num;
	struct internal_type_info_t* data;
};

#define LBIT(x) (((uint64_t) 1) << x)

enum _common_bits_t {
	_M_                     = LBIT(  0 ),
	MOBILE_                 = LBIT(  1 ),
	_MP_                    = LBIT(  2 ),
	_3                      = LBIT(  3 ),
	_5                      = LBIT(  4 ),
	_7                      = LBIT(  5 ),
	_9                      = LBIT(  6 ),
	_H                      = LBIT(  7 ), // powerful mobile processors for laptop
	_S                      = LBIT(  8 ),
	_U                      = LBIT(  9 ), // ultra-low power
	_X                      = LBIT( 10 ), // CPU with great amount of power
	_F                      = LBIT( 11 ), // CPU that doesn’t have integrated graphics
	_G                      = LBIT( 12 ), // CPU with additional built-in integrated graphics
	_E                      = LBIT( 13 ), // Embedded (Intel)
};
#define LAST_COMMON_BIT             13

// additional detection bits for Intel CPUs:
enum _intel_bits_t {
	PENTIUM_                = LBIT( (LAST_COMMON_BIT +  1) ),
	CELERON_                = LBIT( (LAST_COMMON_BIT +  2) ),
	CORE_                   = LBIT( (LAST_COMMON_BIT +  3) ),
	_I_                     = LBIT( (LAST_COMMON_BIT +  4) ),
	XEON_                   = LBIT( (LAST_COMMON_BIT +  5) ),
	ATOM_                   = LBIT( (LAST_COMMON_BIT +  6) ),
	_K                      = LBIT( (LAST_COMMON_BIT +  7) ), // an unlocked desktop processor that allows for overclocking
	_P                      = LBIT( (LAST_COMMON_BIT +  8) ),
	_N                      = LBIT( (LAST_COMMON_BIT +  9) ),
	_W_                     = LBIT( (LAST_COMMON_BIT + 10) ),
	_D_                     = LBIT( (LAST_COMMON_BIT + 11) ),
	_BRONZE_                = LBIT( (LAST_COMMON_BIT + 12) ),
	_SILVER_                = LBIT( (LAST_COMMON_BIT + 13) ),
	_GOLD_                  = LBIT( (LAST_COMMON_BIT + 14) ),
	_PLATINIUM_             = LBIT( (LAST_COMMON_BIT + 15) ),
	_MAX_                   = LBIT( (LAST_COMMON_BIT + 16) ),
	_J_                     = LBIT( (LAST_COMMON_BIT + 17) ),
	_N_                     = LBIT( (LAST_COMMON_BIT + 18) ),
	_ULTRA_                 = LBIT( (LAST_COMMON_BIT + 19) ),
	_V                      = LBIT( (LAST_COMMON_BIT + 20) ), // Lunar Lake
	_L                      = LBIT( (LAST_COMMON_BIT + 21) ), // LGA package (UL = Power efficient, in LGA package / HL = Highest performance, in LGA package)
	_T                      = LBIT( (LAST_COMMON_BIT + 22) ), // Power-optimized lifestyle
	_U_                     = LBIT( (LAST_COMMON_BIT + 23) ),
};
typedef enum _intel_bits_t intel_bits_t;

enum _amd_bits_t {
	ATHLON_      = LBIT( (LAST_COMMON_BIT +  1) ),
	_XP_         = LBIT( (LAST_COMMON_BIT +  2) ),
	DURON_       = LBIT( (LAST_COMMON_BIT +  3) ),
	SEMPRON_     = LBIT( (LAST_COMMON_BIT +  4) ),
	OPTERON_     = LBIT( (LAST_COMMON_BIT +  5) ),
	TURION_      = LBIT( (LAST_COMMON_BIT +  6) ),
	RYZEN_       = LBIT( (LAST_COMMON_BIT +  7) ),
	RYZEN_TR_    = LBIT( (LAST_COMMON_BIT +  8) ),
	EPYC_        = LBIT( (LAST_COMMON_BIT +  9) ),
	_LV_         = LBIT( (LAST_COMMON_BIT + 10) ),
	_64_         = LBIT( (LAST_COMMON_BIT + 11) ),
	_X2          = LBIT( (LAST_COMMON_BIT + 12) ),
	_X3          = LBIT( (LAST_COMMON_BIT + 13) ),
	_X4          = LBIT( (LAST_COMMON_BIT + 14) ),
	_X6          = LBIT( (LAST_COMMON_BIT + 15) ),
	_FX          = LBIT( (LAST_COMMON_BIT + 16) ),
	_APU_        = LBIT( (LAST_COMMON_BIT + 17) ),
	C86_	     = LBIT( (LAST_COMMON_BIT + 18) ),
	_Z           = LBIT( (LAST_COMMON_BIT + 19) ),
	_AI_         = LBIT( (LAST_COMMON_BIT + 20) ),
};
typedef enum _amd_bits_t amd_bits_t;

enum _via_bits_t {
	SAMUEL_            = LBIT( (LAST_COMMON_BIT +  1) ),
	EZRA_              = LBIT( (LAST_COMMON_BIT +  2) ),
	NEHEMIAH_          = LBIT( (LAST_COMMON_BIT +  3) ),
	ESTHER_            = LBIT( (LAST_COMMON_BIT +  4) ),
	EDEN_              = LBIT( (LAST_COMMON_BIT +  5) ),
	CNA_               = LBIT( (LAST_COMMON_BIT +  6) ),
	NANO_              = LBIT( (LAST_COMMON_BIT +  7) ),
	QUADCORE_          = LBIT( (LAST_COMMON_BIT +  8) ),
};
typedef enum _via_bits_t via_bits_t;

enum _zhaoxin_bits_t {
	KAISHENG_          = LBIT( (LAST_COMMON_BIT +  1) ),
	KAIXIAN_           = LBIT( (LAST_COMMON_BIT +  2) ),
	_KH_               = LBIT( (LAST_COMMON_BIT +  3) ),
	_KX_               = LBIT( (LAST_COMMON_BIT +  4) ),
	_ZX_               = LBIT( (LAST_COMMON_BIT +  5) ),
	_C                 = LBIT( (LAST_COMMON_BIT +  6) ),
	_D                 = LBIT( (LAST_COMMON_BIT +  7) ),
};
typedef enum _zhaoxin_bits_t zhaoxin_bits_t;


int cpu_ident_internal(struct cpu_raw_data_t* raw, struct cpu_id_t* data,
		       struct internal_id_info_t* internal);

#endif /* __LIBCPUID_INTERNAL_H__ */
