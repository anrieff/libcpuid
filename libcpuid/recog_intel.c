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
#include <string.h>
#include "libcpuid.h"
#include "recog_intel.h"
#include "libcpuid_util.h"


enum _intel_code_t {
	NA,
	NO_CODE,
	PENTIUM,
	MOBILE_PENTIUM,
	XEON,
	XEON_IRWIN,
	XEONMP,
	XEON_POTOMAC,
	MOBILE_PENTIUM_M,
	CELERON,
	MOBILE_CELERON,
	NOT_CELERON,
	CORE_SOLO,
	CORE_DUO,
	ALLENDALE,
	KENTSFIELD,
	MORE_THAN_QUADCORE,
	PENTIUM_D,
	ATOM_DIAMONDVILLE,
	ATOM_DUALCORE,
	ATOM_SILVERTHORNE,
};
typedef enum _intel_code_t intel_code_t;

const struct match_entry_t cpudb_intel[] = {
	{ -1, -1, -1, -1, -1, NO_CODE           , "Unknown Intel CPU"       },
	
	/* i486 */
	{  4, -1, -1, -1, -1, NO_CODE           , "Unknown i486"            },
	{  4,  0, -1, -1, -1, NO_CODE           , "i486 DX-25/33"           },
	{  4,  1, -1, -1, -1, NO_CODE           , "i486 DX-50"              },
	{  4,  2, -1, -1, -1, NO_CODE           , "i486 SX"                 },
	{  4,  3, -1, -1, -1, NO_CODE           , "i486 DX2"                },
	{  4,  4, -1, -1, -1, NO_CODE           , "i486 SL"                 },
	{  4,  5, -1, -1, -1, NO_CODE           , "i486 SX2"                },
	{  4,  7, -1, -1, -1, NO_CODE           , "i486 DX2 WriteBack"      },
	{  4,  8, -1, -1, -1, NO_CODE           , "i486 DX4"                },
	{  4,  9, -1, -1, -1, NO_CODE           , "i486 DX4 WriteBack"      },
	
	/* All Pentia:
	   Pentium 1 */
	{  5, -1, -1, -1, -1, NO_CODE           , "Unknown Pentium"         },
	{  5,  0, -1, -1, -1, NO_CODE           , "Pentium A-Step"          },
	{  5,  1, -1, -1, -1, NO_CODE           , "Pentium 1 (0.8u)"        },
	{  5,  2, -1, -1, -1, NO_CODE           , "Pentium 1 (0.35u)"       },
	{  5,  3, -1, -1, -1, NO_CODE           , "Pentium OverDrive"       },
	{  5,  4, -1, -1, -1, NO_CODE           , "Pentium 1 (0.35u)"       },
	{  5,  7, -1, -1, -1, NO_CODE           , "Pentium 1 (0.35u)"       },
	{  5,  8, -1, -1, -1, NO_CODE           , "Pentium MMX (0.25u)"     },
	
	/* Pentium 2 / 3 / M / Conroe / whatsnext - all P6 based. */
	{  6, -1, -1, -1, -1, NO_CODE           , "Unknown P6"              },
	{  6,  0, -1, -1, -1, NO_CODE           , "Pentium Pro"             },
	{  6,  1, -1, -1, -1, NO_CODE           , "Pentium Pro"             },
	{  6,  3, -1, -1, -1, NO_CODE           , "Pentium II (Klamath)"    },
	{  6,  5, -1, -1, -1, NO_CODE           , "Pentium II (Deschutes)"  },
	{  6,  6, -1, -1, -1, NO_CODE           , "Pentium II (Dixon)"      },
	
	{  6,  3, -1, -1, -1, XEON              , "P-II Xeon"               },
	{  6,  5, -1, -1, -1, XEON              , "P-II Xeon"               },
	{  6,  6, -1, -1, -1, XEON              , "P-II Xeon"               },
		
	{  6,  5, -1, -1, -1, CELERON           , "P-II Celeron (no L2)"    },
	{  6,  6, -1, -1, -1, CELERON           , "P-II Celeron (128K)"     },
	
	/* ////////////////////////////////////////////////// */
	
	{  6,  7, -1, -1, -1, NO_CODE           , "Pentium III (Katmai)"    },
	{  6,  8, -1, -1, -1, NO_CODE           , "Pentium III (Coppermine)"},
	{  6, 10, -1, -1, -1, NO_CODE           , "Pentium III (Coppermine)"},
	{  6, 11, -1, -1, -1, NO_CODE           , "Pentium III (Tualatin)"  },
	
	{  6,  7, -1, -1, -1, XEON              , "P-III Xeon"              },
	{  6,  8, -1, -1, -1, XEON              , "P-III Xeon"              },
	{  6, 10, -1, -1, -1, XEON              , "P-III Xeon"              },
	{  6, 11, -1, -1, -1, XEON              , "P-III Xeon"              },
	
	{  6,  7, -1, -1, -1, CELERON           , "P-III Celeron"           },
	{  6,  8, -1, -1, -1, CELERON           , "P-III Celeron"           },
	{  6, 10, -1, -1, -1, CELERON           , "P-III Celeron"           },
	{  6, 11, -1, -1, -1, CELERON           , "P-III Celeron"           },
	
	/* ////////////////////////////////////////////////// */
	
	{  6,  9, -1, -1, -1, NO_CODE           , "Unknown Pentium M"       },
	{  6,  9, -1, -1, -1, MOBILE_PENTIUM_M  , "Unknown Pentium M"       },
	{  6,  9, -1, -1, -1, PENTIUM           , "Pentium M (Banias)"      },
	{  6,  9, -1, -1, -1, CELERON           , "Celeron M"               },
	{  6, 13, -1, -1, -1, PENTIUM           , "Pentium M (Dothan)"      },
	{  6, 13, -1, -1, -1, CELERON           , "Celeron M"               },
	
	{  6, 12, -1, -1, -1, NO_CODE           , "Unknown Atom"            },
	{  6, 12, -1, -1, -1, ATOM_DIAMONDVILLE , "Atom (Diamondville)"     },
	{  6, 12, -1, -1, -1, ATOM_DUALCORE     , "Atom (Dual Core)"        },
	{  6, 12, -1, -1, -1, ATOM_SILVERTHORNE , "Atom (Silverthorne)"     },
	
	/* ////////////////////////////////////////////////// */
	
	{  6, 14, -1, -1, -1, NO_CODE           , "Unknown Yonah"           },
	{  6, 14, -1, -1, -1, CORE_SOLO         , "Yonah (Core Solo)"       },
	{  6, 14, -1, -1, -1, CORE_DUO          , "Yonah (Core Duo)"        },
	{  6, 14, -1, -1, -1, XEON              , "Xeon LV"                 },
	{  6, 14, -1, -1, -1, CORE_SOLO         , "Yonah (Core Solo)"       },
	
	{  6, 15, -1, -1, -1, NO_CODE           , "Unknown Core 2"          },
	{  6, 15, -1, -1, -1, CORE_DUO          , "Conroe (Core 2 Duo)"     },
	{  6, 15, -1, -1, -1, KENTSFIELD        , "Kentsfield"              },
	{  6, 15, -1, -1, -1, MORE_THAN_QUADCORE, "More than quad-core"     },
	{  6, 15, -1, -1, -1, ALLENDALE         , "Allendale (Core 2 Duo)"  },
	
	{  6, 16, -1, -1, -1, NO_CODE           , "Unknown Core ?"          }, // future ones
	{  6, 17, -1, -1, -1, NO_CODE           , "Unknown Core ?"          }, // future ones
	{  6, 16, -1, -1, -1, MORE_THAN_QUADCORE, "More than quad-core"     }, // future ones
	{  6, 17, -1, -1, -1, MORE_THAN_QUADCORE, "More than quad-core"     }, // future ones
	
	/* Itaniums */
	{  7, -1, -1, -1, -1, NO_CODE           , "Itanium"                 },
	{ 15, -1, -1,  1, -1, NO_CODE           , "Itanium 2"               },
	
	/* Netburst based (Pentium 4 and later)
	   classic P4s */
	{ 15, -1, -1,  0, -1, NO_CODE           , "Unknown Pentium 4"       },
	{ 15, -1, -1,  0, -1, CELERON           , "Unknown P-4 Celeron"     },
	{ 15, -1, -1,  0, -1, XEON              , "Unknown Xeon"            },
	
	{ 15,  0, -1,  0, -1, NO_CODE           , "Pentium 4 (Willamette)"  },
	{ 15,  1, -1,  0, -1, NO_CODE           , "Pentium 4 (Willamette)"  },
	{ 15,  2, -1,  0, -1, NO_CODE           , "Pentium 4 (Northwood)"   },
	{ 15,  3, -1,  0, -1, NO_CODE           , "Pentium 4 (Prescott)"    },
	{ 15,  4, -1,  0, -1, NO_CODE           , "Pentium 4 (Prescott)"    },
	
	/* server CPUs */
	{ 15,  0, -1,  0, -1, XEON              , "Xeon (Foster)"           },
	{ 15,  1, -1,  0, -1, XEON              , "Xeon (Foster)"           },
	{ 15,  2, -1,  0, -1, XEON              , "Xeon (Prestonia)"        },
	{ 15,  2, -1,  0, -1, XEONMP            , "Xeon (Gallatin)"         },
	{ 15,  3, -1,  0, -1, XEON              , "Xeon (Nocona)"           },
	{ 15,  4, -1,  0, -1, XEON              , "Xeon (Nocona)"           },
	{ 15,  4, -1,  0, -1, XEON_IRWIN        , "Xeon (Irwindale)"        },
	{ 15,  4, -1,  0, -1, XEONMP            , "Xeon (Cranford)"         },
	{ 15,  4, -1,  0, -1, XEON_POTOMAC      , "Xeon (Potomac)"          },
	{ 15,  6, -1,  0, -1, XEON              , "Xeon 5000"               },
	
	/* Pentium Ds */
	{ 15,  4,  4,  0, -1, NO_CODE           , "Pentium D"               },
	{ 15,  4, -1,  0, -1, PENTIUM_D         , "Pentium D"               },
	{ 15,  4,  7,  0, -1, NO_CODE           , "Pentium D"               },
	{ 15,  6, -1,  0, -1, PENTIUM_D         , "Pentium D"               },

	/* Celeron and Celeron Ds */
	{ 15,  1, -1,  0, -1, CELERON           , "P-4 Celeron (128K)"      }, 
	{ 15,  2, -1,  0, -1, CELERON           , "P-4 Celeron (128K)"      },
	{ 15,  3, -1,  0, -1, CELERON           , "Celeron D"               },
	{ 15,  4, -1,  0, -1, CELERON           , "Celeron D"               },
	{ 15,  6, -1,  0, -1, CELERON           , "Celeron D"               },
};


static void load_intel_features(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	const struct feature_map_t matchtable_edx1[] = {
		{ 18, CPU_FEATURE_PN },
		{ 21, CPU_FEATURE_DTS },
		{ 22, CPU_FEATURE_ACPI },
		{ 27, CPU_FEATURE_SS },
		{ 29, CPU_FEATURE_TM },
		{ 30, CPU_FEATURE_IA64 },
		{ 31, CPU_FEATURE_PBE },
	};
	const struct feature_map_t matchtable_ecx1[] = {
		{  1, CPU_FEATURE_PCLMUL },
		{  2, CPU_FEATURE_DTS64 },
		{  4, CPU_FEATURE_DS_CPL },
		{  5, CPU_FEATURE_VMX },
		{  6, CPU_FEATURE_SMX },
		{  7, CPU_FEATURE_EST },
		{  8, CPU_FEATURE_TM2 },
		{ 10, CPU_FEATURE_CID },
		{ 14, CPU_FEATURE_XTPR },
		{ 15, CPU_FEATURE_PDCM },
		{ 18, CPU_FEATURE_DCA },
		{ 20, CPU_FEATURE_SSE4_2 },
		{ 22, CPU_FEATURE_MOVBE },
		{ 25, CPU_FEATURE_AES },
		{ 26, CPU_FEATURE_XSAVE },
		{ 27, CPU_FEATURE_OSXSAVE },
		{ 28, CPU_FEATURE_AVX },
	};
	const struct feature_map_t matchtable_edx81[] = {
		{ 20, CPU_FEATURE_XD },
	};
	if (raw->basic_cpuid[0][0] >= 1) {
		match_features(matchtable_edx1, COUNT_OF(matchtable_edx1), raw->basic_cpuid[1][3], data);
		match_features(matchtable_ecx1, COUNT_OF(matchtable_ecx1), raw->basic_cpuid[1][2], data);
	}
	if (raw->ext_cpuid[0][0] >= 1) {
		match_features(matchtable_edx81, COUNT_OF(matchtable_edx81), raw->ext_cpuid[1][3], data);
	}
}

enum _cache_type_t {
	L1I,
	L1D,
	L2,
	L3
};
typedef enum _cache_type_t cache_type_t;

static void check_case(uint8_t on, cache_type_t cache, int size, int assoc, int linesize, struct cpu_id_t* data)
{
	if (!on) return;
	switch (cache) {
		case L1I:
			data->l1_instruction_cache = size;
			break;
		case L1D:
			data->l1_data_cache = size;
			data->l1_assoc = assoc;
			data->l1_cacheline = linesize;
			break;
		case L2:
			data->l2_cache = size;
			data->l2_assoc = assoc;
			data->l2_cacheline = linesize;
			break;
		case L3:
			data->l3_cache = size;
			data->l3_assoc = assoc;
			data->l3_cacheline = linesize;
		default:
			break;
	}
}

static void decode_intel_oldstyle_cache_info(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	uint8_t f[256] = {0};
	int reg, off;
	uint32_t x;
	for (reg = 0; reg < 4; reg++) {
		x = raw->basic_cpuid[2][reg];
		if (x & 0x80000000) continue;
		for (off = 0; off < 4; off++) {
			f[x & 0xff] = 1;
			x >>= 8;
		}
	}
	
	check_case(f[0x06], L1I,      8,  4,  32, data);
	check_case(f[0x08], L1I,     16,  4,  32, data);
	check_case(f[0x0A], L1D,      8,  2,  32, data);
	check_case(f[0x0C], L1D,     16,  4,  32, data);
	check_case(f[0x22],  L3,    512,  4,  64, data);
	check_case(f[0x23],  L3,   1024,  8,  64, data);
	check_case(f[0x25],  L3,   2048,  8,  64, data);
	check_case(f[0x29],  L3,   4096,  8,  64, data);
	check_case(f[0x2C], L1D,     32,  8,  64, data);
	check_case(f[0x30], L1I,     32,  8,  64, data);
	check_case(f[0x39],  L2,    128,  4,  64, data);
	check_case(f[0x3A],  L2,    192,  6,  64, data);
	check_case(f[0x3B],  L2,    128,  2,  64, data);
	check_case(f[0x3C],  L2,    256,  4,  64, data);
	check_case(f[0x3D],  L2,    384,  6,  64, data);
	check_case(f[0x3E],  L2,    512,  4,  64, data);
	check_case(f[0x41],  L2,    128,  4,  32, data);
	check_case(f[0x42],  L2,    256,  4,  32, data);
	check_case(f[0x43],  L2,    512,  4,  32, data);
	check_case(f[0x44],  L2,   1024,  4,  32, data);
	check_case(f[0x45],  L2,   2048,  4,  32, data);
	check_case(f[0x46],  L3,   4096,  4,  64, data);
	check_case(f[0x47],  L3,   8192,  8,  64, data);
	check_case(f[0x4A],  L3,   6144, 12,  64, data);
	check_case(f[0x4B],  L3,   8192, 16,  64, data);
	check_case(f[0x4C],  L3,  12288, 12,  64, data);
	check_case(f[0x4D],  L3,  16384, 16,  64, data);
	check_case(f[0x4E],  L2,   6144, 24,  64, data);
	check_case(f[0x60], L1D,     16,  8,  64, data);
	check_case(f[0x66], L1D,      8,  4,  64, data);
	check_case(f[0x67], L1D,     16,  4,  64, data);
	check_case(f[0x68], L1D,     32,  4,  64, data);
	/* The following four entries are trace cache. Intel does not
	 * specify a cache-line size, so we use -1 instead
	 */
	check_case(f[0x70], L1I,     12,  8,  -1, data);
	check_case(f[0x71], L1I,     16,  8,  -1, data);
	check_case(f[0x72], L1I,     32,  8,  -1, data);
	check_case(f[0x73], L1I,     64,  8,  -1, data);
	
	check_case(f[0x78],  L2,   1024,  4,  64, data);
	check_case(f[0x79],  L2,    128,  8,  64, data);
	check_case(f[0x7A],  L2,    256,  8,  64, data);
	check_case(f[0x7B],  L2,    512,  8,  64, data);
	check_case(f[0x7C],  L2,   1024,  8,  64, data);
	check_case(f[0x7D],  L2,   2048,  8,  64, data);
	check_case(f[0x7F],  L2,    512,  2,  64, data);
	check_case(f[0x82],  L2,    256,  8,  32, data);
	check_case(f[0x83],  L2,    512,  8,  32, data);
	check_case(f[0x84],  L2,   1024,  8,  32, data);
	check_case(f[0x85],  L2,   2048,  8,  32, data);
	check_case(f[0x86],  L2,    512,  4,  64, data);
	check_case(f[0x87],  L2,   1024,  8,  64, data);
	
	if (f[0x49]) {
		/* This flag is overloaded with two meanings. On Xeon MP
		 * (family 0xf, model 0x6) this means L3 cache. On all other
		 * CPUs (notably Conroe et al), this is L2 cache. In both cases
		 * it means 4MB, 16-way associative, 64-byte line size.
		 */
		if (data->family == 0xf && data->model == 0x6) {
			data->l3_cache = 4096;
			data->l3_assoc = 16;
			data->l3_cacheline = 64;
		} else {
			data->l2_cache = 4096;
			data->l2_assoc = 16;
			data->l2_cacheline = 64;
		}
	}
	if (f[0x40]) {
		/* Again, a special flag. It means:
		 * 1) If no L2 is specified, then CPU is w/o L2 (0 KB)
		 * 2) If L2 is specified by other flags, then, CPU is w/o L3.
		 */
		if (data->l2_cache == -1) {
			data->l2_cache = 0;
		} else {
			data->l3_cache = 0;
		}
	}
}

static void decode_intel_deterministic_cache_info(struct cpu_raw_data_t* raw,
                                                  struct cpu_id_t* data)
{
	int ecx;
	int ways, partitions, linesize, sets, size, level, typenumber;
	cache_type_t type;
	for (ecx = 0; ecx < MAX_INTELFN4_LEVEL; ecx++) {
		typenumber = raw->intel_fn4[ecx][0] & 0x1f;
		if (typenumber == 0) break;
		level = (raw->intel_fn4[ecx][0] >> 5) & 0x7;
		if (level == 1 && typenumber == 1)
			type = L1D;
		else if (level == 1 && typenumber == 2)
			type = L1I;
		else if (level == 2 && typenumber == 3)
			type = L2;
		else if (level == 3 && typenumber == 3)
			type = L3;
		else {
			warnf("deterministic_cache: unknown level/typenumber combo (%d/%d), cannot\n", level, typenumber);
			warnf("deterministic_cache: recognize cache type\n");
			continue;
		}
		ways = ((raw->intel_fn4[ecx][1] >> 22) & 0x3ff) + 1;
		partitions = ((raw->intel_fn4[ecx][1] >> 12) & 0x3ff) + 1;
		linesize = (raw->intel_fn4[ecx][1] & 0xfff) + 1;
		sets = raw->intel_fn4[ecx][2] + 1;
		size = ways * partitions * linesize * sets / 1024;
		check_case(1, type, size, ways, linesize, data);
	}
}

static void decode_intel_number_of_cores(struct cpu_raw_data_t* raw,
                                         struct cpu_id_t* data)
{
	int logical_cpus = -1, num_cores = -1;
	
	if (raw->basic_cpuid[0][0] >= 1) {
		logical_cpus = (raw->basic_cpuid[1][1] >> 16) & 0xff;
		if (raw->basic_cpuid[0][0] >= 4) {
			num_cores = 1 + ((raw->basic_cpuid[4][0] >> 26) & 0x3f);
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

static void decode_intel_codename(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	intel_code_t code = NO_CODE;
	int i;
	const char* bs = data->brand_str;
	if (strstr(bs, "Mobile")) {
		if (strstr(bs, "Celeron"))
			code = MOBILE_CELERON;
		else if (strstr(bs, "Pentium"))
			code = MOBILE_PENTIUM;
	} else {
		const struct { intel_code_t c; const char *search; }
		matchtable[] = {
			{ XEONMP, "Xeon MP" },
			{ XEONMP, "Xeon(TM) MP" },
			{ XEON, "Xeon" },
			{ CELERON, "Celeron" },
			{ MOBILE_PENTIUM_M, "Pentium(R) M" },
			{ PENTIUM_D, "Pentium(R) D" },
			{ PENTIUM, "Pentium" },
			{ CORE_SOLO, "Genuine Intel(R) CPU" },
			{ CORE_SOLO, "Intel(R) Core(TM)2" },
			{ ATOM_DIAMONDVILLE, "Atom(TM) CPU 2" },
			{ ATOM_DIAMONDVILLE, "Atom(TM) CPU N" },
			{ ATOM_DUALCORE, "Atom(TM) CPU 3" },
			{ ATOM_SILVERTHORNE, "Atom(TM) CPU Z" },
		};
		for (i = 0; i < COUNT_OF(matchtable); i++)
			if (strstr(bs, matchtable[i].search)) {
				code = matchtable[i].c;
				break;
			}
	}
	
	if (code == XEON && data->l3_cache > 0)
		code = XEON_IRWIN;
	if (code == XEONMP && data->l3_cache > 0)
		code = XEON_POTOMAC;
	if (code == CORE_SOLO) {
		switch (data->num_cores) {
			case 1: break;
			case 2: code = CORE_DUO; break;
			case 4: code = KENTSFIELD; break;
			default:
				code = MORE_THAN_QUADCORE; break;
		}
	}
	if (code == CORE_DUO && data->l2_cache == 2048)
		code = ALLENDALE;
	match_cpu_codename(cpudb_intel, COUNT_OF(cpudb_intel), data, code);
}

int cpuid_identify_intel(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	load_intel_features(raw, data);
	if (raw->basic_cpuid[0][0] >= 4) {
		/* Deterministic way is preferred, being more generic */
		decode_intel_deterministic_cache_info(raw, data);
	} else if (raw->basic_cpuid[0][0] >= 2) {
		decode_intel_oldstyle_cache_info(raw, data);
	}
	decode_intel_number_of_cores(raw, data);
	decode_intel_codename(raw, data);
	return 0;
}
