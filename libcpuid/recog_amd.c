/*
 * Copyright 2008  Veselin Georgiev,
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

#include <stdio.h>
#include <string.h>
#include "libcpuid.h"
#include "recog_amd.h"
#include "libcpuid_util.h"

enum _amd_code_t {
	NA,
	NO_CODE,
	OPTERON_GENERIC,
	OPTERON_800,
	ATHLON_XP,
	ATHLON_XP_BARTON,
	ATHLON_XP_M,
	ATHLON_XP_M_LV,
	ATHLON,
	ATHLON_MP,
	MOBILE_ATHLON64,
	ATHLON_FX,
	DURON,
	DURON_MP,
	MOBILE_DURON,
	SEMPRON,
	MOBILE_SEMPRON,
	OPTERON_SINGLE,
	OPTERON_DUALCORE,
	OPTERON_800_DUALCORE,
	MOBILE_TURION,
	ATHLON_64_512K,
	ATHLON_64_1M,
	ATHLON_64_X2_512K,
	ATHLON_64_X2_1M,
	ATHLON_64_FX,
	TURION_64_512K,
	TURION_64_1M,
	TURION_X2_512K,
	TURION_X2_1M,
	SEMPRON_64_128K,
	SEMPRON_64_256K,
	SEMPRON_64_512K,
	M_SEMPRON_64_256K,
	M_SEMPRON_64_512K,
	SEMPRON_DUALCORE,
	PHENOM,
	PHENOM_X2,
	PHENOM_X3,
	PHENOM_X4,
};
typedef enum _amd_code_t amd_code_t;

const struct match_entry_t cpudb_amd[] = {
	{ -1, -1, -1, -1, -1, NO_CODE          , "Unknown AMD CPU"         },
	
	/* 486 and the likes */
	{  4, -1, -1, -1, -1, NO_CODE          , "Unknown AMD 486"         },
	{  4,  3, -1, -1, -1, NO_CODE          , "AMD 486DX2"              },
	{  4,  7, -1, -1, -1, NO_CODE          , "AMD 486DX2WB"            },
	{  4,  8, -1, -1, -1, NO_CODE          , "AMD 486DX4"              },
	{  4,  9, -1, -1, -1, NO_CODE          , "AMD 486DX4WB"            },
	
	/* Pentia clones */
	{  5, -1, -1, -1, -1, NO_CODE          , "Unknown AMD 586"         },
	{  5,  0, -1, -1, -1, NO_CODE          , "K5"                      },
	{  5,  1, -1, -1, -1, NO_CODE          , "K5"                      },
	{  5,  2, -1, -1, -1, NO_CODE          , "K5"                      },
	{  5,  3, -1, -1, -1, NO_CODE          , "K5"                      },
	
	/* The K6 */
	{  5,  6, -1, -1, -1, NO_CODE          , "K6"                      },
	{  5,  7, -1, -1, -1, NO_CODE          , "K6"                      },
	
	{  5,  8, -1, -1, -1, NO_CODE          , "K6-2"                    },
	{  5,  9, -1, -1, -1, NO_CODE          , "K6-III"                  },
	{  5, 10, -1, -1, -1, NO_CODE          , "Unknown K6"              },
	{  5, 11, -1, -1, -1, NO_CODE          , "Unknown K6"              },
	{  5, 12, -1, -1, -1, NO_CODE          , "Unknown K6"              },
	{  5, 13, -1, -1, -1, NO_CODE          , "K6-2+"                   },
	
	/* Athlon et al. */
	{  6,  1, -1, -1, -1, NO_CODE          , "Athlon (Slot-A)"         },
	{  6,  2, -1, -1, -1, NO_CODE          , "Athlon (Slot-A)"         },
	{  6,  3, -1, -1, -1, NO_CODE          , "Duron (Spitfire)"        },
	{  6,  4, -1, -1, -1, NO_CODE          , "Athlon (ThunderBird)"    },
	
	{  6,  6, -1, -1, -1, NO_CODE          , "Unknown Athlon"          },
	{  6,  6, -1, -1, -1, ATHLON           , "Athlon (Palomino)"       },
	{  6,  6, -1, -1, -1, ATHLON_MP        , "Athlon MP (Palomino)"    },
	{  6,  6, -1, -1, -1, DURON            , "Duron (Palomino)"        },
	{  6,  6, -1, -1, -1, ATHLON_XP        , "Athlon XP"               },
	
	{  6,  7, -1, -1, -1, NO_CODE          , "Unknown Athlon XP"       },
	{  6,  7, -1, -1, -1, DURON            , "Duron (Morgan)"          },
	
	{  6,  8, -1, -1, -1, NO_CODE          , "Athlon XP"               },
	{  6,  8, -1, -1, -1, ATHLON           , "Athlon XP"               },
	{  6,  8, -1, -1, -1, ATHLON_XP        , "Athlon XP"               },
	{  6,  8, -1, -1, -1, DURON            , "Duron (Applebred)"       },
	{  6,  8, -1, -1, -1, SEMPRON          , "Sempron (Thoroughbred)"  },
	{  6,  8, -1, -1, -1, SEMPRON_64_128K  , "Sempron (Thoroughbred)"  },
	{  6,  8, -1, -1, -1, SEMPRON_64_256K  , "Sempron (Thoroughbred)"  },
	{  6,  8, -1, -1, -1, ATHLON_MP        , "Athlon MP (Thoroughbred)"},
	{  6,  8, -1, -1, -1, ATHLON_XP_M      , "Mobile Athlon (T-Bred)"  },
	{  6,  8, -1, -1, -1, ATHLON_XP_M_LV   , "Mobile Athlon (T-Bred)"  },
	
	{  6, 10, -1, -1, -1, NO_CODE          , "Athlon XP (Barton)"      },
	{  6, 10, -1, -1, -1, ATHLON           , "Athlon XP (Barton)"      },
	{  6, 10, -1, -1, -1, ATHLON_XP_BARTON , "Athlon XP (Barton)"      },
	{  6, 10, -1, -1, -1, SEMPRON          , "Sempron (Barton)"        },
	{  6, 10, -1, -1, -1, SEMPRON_64_256K  , "Sempron (Barton)"        },
	{  6, 10, -1, -1, -1, ATHLON_XP        , "Athlon XP"               },
	/* ^^ Actually, Thorton, but it's equivallent to Thoroughbred */
	{  6, 10, -1, -1, -1, ATHLON_MP        , "Athlon MP (Barton)"      },
	{  6, 10, -1, -1, -1, ATHLON_XP_M      , "Mobile Athlon (Barton)"  },
	{  6, 10, -1, -1, -1, ATHLON_XP_M_LV   , "Mobile Athlon (Barton)"  },
	
	/* K8 Architecture */
	{ 15, -1, -1, 15, -1, NO_CODE          , "Unknown K8"              },
	{ 15, -1, -1, 16, -1, NO_CODE          , "Unknown K9"              },
	
	{ 15, -1, -1, 15, -1, NO_CODE          , "Unknown A64"             },
	{ 15, -1, -1, 15, -1, OPTERON_SINGLE   , "Opteron"                 },
	{ 15, -1, -1, 15, -1, OPTERON_DUALCORE , "Opteron (Dual Core)"     },
	{ 15,  3, -1, 15, -1, OPTERON_SINGLE   , "Opteron"                 },
	{ 15,  3, -1, 15, -1, OPTERON_DUALCORE , "Opteron (Dual Core)"     },
	{ 15, -1, -1, 15, -1, ATHLON_64_512K   , "Athlon 64 (512K)"        },
	{ 15, -1, -1, 15, -1, ATHLON_64_1M     , "Athlon 64 (1024K)"       },
	{ 15, -1, -1, 15, 0x2c, ATHLON_64_512K   , "Athlon 64 (Venice/512K)"        },
	{ 15, -1, -1, 15, -1, ATHLON_64_X2_512K, "Athlon 64 X2 (512K)"     },
	{ 15, -1, -1, 15, 0x6b, ATHLON_64_X2_512K, "Athlon 64 X2 (Brisbane/512K)"     },
	{ 15, -1, -1, 15, -1, ATHLON_64_X2_1M  , "Athlon 64 X2 (1024K)"    },
	{ 15, -1, -1, 15, -1, ATHLON_FX        , "Athlon FX"               },
	{ 15, -1, -1, 15, -1, ATHLON_64_FX     , "Athlon 64 FX"            },
	{ 15, -1, -1, 15, -1, TURION_64_512K   , "Turion 64 (512K)"        },
	{ 15, -1, -1, 15, -1, TURION_64_1M     , "Turion 64 (1024K)"       },
	{ 15, -1, -1, 15, -1, TURION_X2_512K   , "Turion 64 X2 (512K)"     },
	{ 15, -1, -1, 15, -1, TURION_X2_1M     , "Turion 64 X2 (1024K)"    },
	{ 15, -1, -1, 15, -1, SEMPRON_64_128K  , "A64 Sempron (128K)"      },
	{ 15, -1, -1, 15, -1, SEMPRON_64_256K  , "A64 Sempron (256K)"      },
	{ 15, -1, -1, 15, -1, SEMPRON_64_512K  , "A64 Sempron (512K)"      },
	{ 15, -1, -1, 15, 0x2c, SEMPRON_64_128K, "Sempron 64 (Palermo/128K)"      },
	{ 15, -1, -1, 15, 0x2c, SEMPRON_64_256K, "Sempron 64 (Palermo/256K)"      },
	{ 15, -1, -1, 15, 0x2f, SEMPRON_64_128K, "Sempron 64 (Palermo/128K)"      },
	{ 15, -1, -1, 15, 0x2f, SEMPRON_64_256K, "Sempron 64 (Palermo/256K)"      },
	{ 15, -1, -1, 15, 0x4f, SEMPRON_64_128K, "Sempron 64 (Manila/128K)"      },
	{ 15, -1, -1, 15, 0x4f, SEMPRON_64_256K, "Sempron 64 (Manila/256K)"      },
	{ 15, -1, -1, 15, 0x7f, SEMPRON_64_256K, "Sempron 64 (Sparta/256K)"      },
	{ 15, -1, -1, 15, 0x7f, SEMPRON_64_512K, "Sempron 64 (Sparta/512K)"      },
	{ 15, -1, -1, 15, -1, M_SEMPRON_64_256K, "Mobile Sempron 64 (Keene/256K)"  },
	{ 15, -1, -1, 15, -1, M_SEMPRON_64_512K, "Mobile Sempron 64 (Keene/512K)"  },
	{ 15, -1, -1, 15, -1, SEMPRON_DUALCORE , "A64 Sempron (Dual Core)" },
	
	/* K9 Architecture */
	{ 15, -1, -1, 16, -1, PHENOM           , "Unknown AMD Phenom"      },
	{ 15,  2, -1, 16, -1, PHENOM           , "Phenom"                  },
	{ 15,  2, -1, 16, -1, PHENOM_X3        , "Phenom X3 (Toliman)"     },
	{ 15,  2, -1, 16, -1, PHENOM_X4        , "Phenom X4 (Agena)"       },
};


static void load_amd_features(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	const struct feature_map_t matchtable_edx81[] = {
		{ 20, CPU_FEATURE_NX },
		{ 22, CPU_FEATURE_MMXEXT },
		{ 25, CPU_FEATURE_FXSR_OPT },
		{ 27, CPU_FEATURE_RDTSCP },
		{ 30, CPU_FEATURE_3DNOWEXT },
		{ 31, CPU_FEATURE_3DNOW },
	};
	const struct feature_map_t matchtable_ecx81[] = {
		{  1, CPU_FEATURE_CMP_LEGACY },
		{  2, CPU_FEATURE_SVM },
		{  5, CPU_FEATURE_ABM },
		{  6, CPU_FEATURE_SSE4A },
		{  7, CPU_FEATURE_MISALIGNSSE },
		{  8, CPU_FEATURE_3DNOWPREFETCH },
		{  9, CPU_FEATURE_OSVW },
		{ 10, CPU_FEATURE_IBS },
		{ 11, CPU_FEATURE_SSE5 },
		{ 12, CPU_FEATURE_SKINIT },
		{ 13, CPU_FEATURE_WDT },
	};
	const struct feature_map_t matchtable_edx87[] = {
		{  0, CPU_FEATURE_TS },
		{  1, CPU_FEATURE_FID },
		{  2, CPU_FEATURE_VID },
		{  3, CPU_FEATURE_TTP },
		{  4, CPU_FEATURE_TM_AMD },
		{  5, CPU_FEATURE_STC },
		{  6, CPU_FEATURE_100MHZSTEPS },
		{  7, CPU_FEATURE_HWPSTATE },
		{  8, CPU_FEATURE_CONSTANT_TSC },
	};
	if (raw->ext_cpuid[0][0] >= 1) {
		match_features(matchtable_edx81, COUNT_OF(matchtable_edx81), raw->ext_cpuid[1][3], data);
		match_features(matchtable_ecx81, COUNT_OF(matchtable_ecx81), raw->ext_cpuid[1][2], data);
	}
	if (raw->ext_cpuid[0][0] >= 7)
		match_features(matchtable_edx87, COUNT_OF(matchtable_edx87), raw->ext_cpuid[7][3], data);
}

static void decode_amd_cache_info(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int l3_result;
	const int assoc_table[16] = {
		0, 1, 2, 0, 4, 0, 8, 0, 16, 0, 32, 48, 64, 92, 128, 255
	};
	unsigned n = raw->ext_cpuid[0][0];
	
	if (n >= 0x80000005) {
		data->l1_data_cache = (raw->ext_cpuid[5][2] >> 24) & 0xff;
		data->l1_assoc = (raw->ext_cpuid[5][2] >> 16) & 0xff;
		data->l1_cacheline = (raw->ext_cpuid[5][2]) & 0xff;
		data->l1_instruction_cache = (raw->ext_cpuid[5][3] >> 24) & 0xff;
	}
	if (n >= 0x80000006) {
		data->l2_cache = (raw->ext_cpuid[6][2] >> 16) & 0xffff;
		data->l2_assoc = assoc_table[(raw->ext_cpuid[6][2] >> 12) & 0xf];
		data->l2_cacheline = (raw->ext_cpuid[6][2]) & 0xff;
		
		l3_result = (raw->ext_cpuid[6][3] >> 18);
		if (l3_result > 0) {
			l3_result = 512 * l3_result; /* AMD spec says it's a range,
			                                but we take the lower bound */
			data->l3_cache = l3_result;
			data->l3_assoc = assoc_table[(raw->ext_cpuid[6][3] >> 12) & 0xf];
			data->l3_cacheline = (raw->ext_cpuid[6][3]) & 0xff;
		} else {
			data->l3_cache = 0;
		}
	}
}

static void decode_amd_number_of_cores(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int logical_cpus = -1, num_cores = -1;
	
	if (raw->basic_cpuid[0][0] >= 1) {
		logical_cpus = (raw->basic_cpuid[1][1] >> 16) & 0xff;
		if (raw->ext_cpuid[0][0] >= 8) {
			num_cores = 1 + (raw->ext_cpuid[8][2] & 0xff);
		}
	}
	if (data->flags[CPU_FEATURE_HT]) {
		if (num_cores > 1) {
			data->num_cores = num_cores;
			data->num_logical_cpus = logical_cpus;
		} else {
			data->num_cores = 1;
			data->num_logical_cpus = (logical_cpus >= 2 ? logical_cpus : 2);
		}
	} else {
		data->num_cores = data->num_logical_cpus = 1;
	}
}

static amd_code_t decode_amd_codename_part1(const char *bs)
{
	if (strstr(bs, "Opteron")) {
		if (strstr(bs, "Dual Core")) return OPTERON_DUALCORE;
		return OPTERON_SINGLE;
	}
	if (strstr(bs, "Phenom")) {
		return PHENOM;
	}
	if (strstr(bs, "Athlon(tm) 64 FX")) return ATHLON_64_FX;
	if (strstr(bs, "Athlon(tm) FX")) return ATHLON_FX;
	if (strstr(bs, "Athlon(tm) 64")) {
		if (strstr(bs, "Dual Core")) return ATHLON_64_X2_512K;
		return ATHLON_64_512K;
	}
	if (strstr(bs, "Athlon(tm) X2")) {
		return ATHLON_64_X2_512K;
	}
	if (strstr(bs, "Turion(tm)")) {
		if (strstr(bs, "X2"))
			return TURION_X2_512K;
		else
			return TURION_64_512K;
	}
	
	if (strstr(bs, "mobile") || strstr(bs, "Mobile")) {
		if (strstr(bs, "Athlon(tm) XP-M (LV)")) return ATHLON_XP_M_LV;
		if (strstr(bs, "Athlon(tm) XP")) return ATHLON_XP_M;
		if (strstr(bs, "Sempron(tm)")) return M_SEMPRON_64_256K;
		if (strstr(bs, "Athlon")) return MOBILE_ATHLON64;
		if (strstr(bs, "Duron")) return MOBILE_DURON;
		
	} else {
		if (strstr(bs, "Athlon(tm) XP")) return ATHLON_XP;
		if (strstr(bs, "Athlon(tm) MP")) return ATHLON_MP;
		if (strstr(bs, "Sempron(tm)")) return SEMPRON_64_128K;
		if (strstr(bs, "Duron")) return DURON;
		if (strstr(bs, "Athlon")) return ATHLON;
	}
	
	return NO_CODE;
}

static void decode_amd_codename(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	amd_code_t code = decode_amd_codename_part1(data->brand_str);
	
	if (code == ATHLON_XP && data->l2_cache == 512)
		code = ATHLON_XP_BARTON;
	if (code == ATHLON_64_512K && data->l2_cache > 512)
		code = ATHLON_64_1M;
	if (code == SEMPRON_64_128K && data->l2_cache > 128) {
		if (data->l2_cache == 256)
			code = SEMPRON_64_256K;
		else
			code = SEMPRON_64_512K;
	}
	if (code == M_SEMPRON_64_256K && data->l2_cache > 256)
		code = M_SEMPRON_64_512K;
	if (code == TURION_64_512K && data->l2_cache > 512)
		code = TURION_64_1M;
	if (code == TURION_X2_512K && data->l2_cache > 512)
		code = TURION_X2_1M;
	if (code == ATHLON_64_X2_512K && data->l2_cache > 512)
		code = ATHLON_64_X2_1M;
	if (code == ATHLON_64_X2_512K && data->l2_cache < 512)
		code = SEMPRON_DUALCORE;
	if (code == PHENOM) {
		switch (data->num_cores) {
			case 2:
				code = PHENOM_X2;
				break;
			case 3:
				code = PHENOM_X3;
				break;
			case 4:
				code = PHENOM_X4;
				break;
			default:
				break;
		}
	}
	match_cpu_codename(cpudb_amd, COUNT_OF(cpudb_amd), data, code);
}

int cpuid_identify_amd(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	load_amd_features(raw, data);
	decode_amd_cache_info(raw, data);
	decode_amd_number_of_cores(raw, data);
	decode_amd_codename(raw, data);
	return 0;
}
