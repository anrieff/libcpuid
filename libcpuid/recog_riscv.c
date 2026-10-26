/*
 * Copyright 2026  Veselin Georgiev,
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
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "libcpuid.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"
#include "recog_riscv.h"


struct riscv_part_t {
	/* Machine Architecture ID (marchid CSR), 0 when it does not identify the core */
	const uint64_t marchid;
	/* "compatible" string of the CPU node in the device tree (the "uarch" field of /proc/cpuinfo on Linux),
	   NULL when unknown. Used to identify cores where marchid is not meaningful (e.g. T-Head) */
	const char* uarch;
	const char* name;
	const char* codename;
	const char* technology;
	const cpu_purpose_t purpose;
};

struct riscv_vendor_t {
	/* Machine Vendor ID (mvendorid CSR): JEDEC manufacturer ID, bank number in bits [31:7] */
	const int64_t mvendorid;
	const cpu_vendor_t vendor;
	const struct riscv_part_t *parts;
	const char *name;
};

/* Open-source architecture IDs: https://github.com/riscv/riscv-isa-manual/blob/main/marchid.md
   Also reported with mvendorid 0 by QEMU (marchid 0) */
static const struct riscv_part_t open_source_part[] = {
	{  1, NULL,                     "Rocket",     UNKN_STR, UNKN_STR, PURPOSE_GENERAL },
	{  2, NULL,                     "BOOM",       UNKN_STR, UNKN_STR, PURPOSE_GENERAL },
	{  3, NULL,                     "CVA6",       UNKN_STR, UNKN_STR, PURPOSE_GENERAL },
	{  5, NULL,                     "Spike",      UNKN_STR, UNKN_STR, PURPOSE_GENERAL },
	{ 22, NULL,                     "Ibex",       UNKN_STR, UNKN_STR, PURPOSE_GENERAL },
	{ 25, NULL,                     "XiangShan",  UNKN_STR, UNKN_STR, PURPOSE_GENERAL },
	{ 27, NULL,                     "Hazard3",    UNKN_STR, UNKN_STR, PURPOSE_GENERAL },
	{ 33, NULL,                     "VexRiscv",   UNKN_STR, UNKN_STR, PURPOSE_GENERAL },
	{  0, NULL,                     NULL,         NULL,     NULL,     PURPOSE_GENERAL },
};

/* Commercial IDs: https://github.com/llvm/llvm-project/blob/main/llvm/lib/Target/RISCV/RISCVProcessors.td */
static const struct riscv_part_t sifive_part[] = {
	{ 0x8000000000000001ULL, "sifive,u54-mc",   "U54",   UNKN_STR, UNKN_STR, PURPOSE_GENERAL     },
	{ 0x8000000000000007ULL, "sifive,u74-mc",   "U74",   UNKN_STR, UNKN_STR, PURPOSE_GENERAL     },
	{ 0x8000000000000008ULL, "sifive,p550",     "P550",  UNKN_STR, UNKN_STR, PURPOSE_PERFORMANCE },
	{ 0x8000000000000109ULL, "sifive,p870-d",   "P870-D", UNKN_STR, UNKN_STR, PURPOSE_PERFORMANCE },
	{ 0,                     NULL,              NULL,    NULL,     NULL,     PURPOSE_GENERAL     },
};

/* T-Head cores report marchid 0, so they are identified by the device tree only */
static const struct riscv_part_t thead_part[] = {
	{ 0, "thead,c906",   "C906",   "XuanTie", UNKN_STR, PURPOSE_GENERAL     },
	{ 0, "thead,c908",   "C908",   "XuanTie", UNKN_STR, PURPOSE_GENERAL     },
	{ 0, "thead,c910",   "C910",   "XuanTie", UNKN_STR, PURPOSE_PERFORMANCE },
	{ 0, "thead,c920",   "C920",   "XuanTie", UNKN_STR, PURPOSE_PERFORMANCE },
	{ 0, "thead,c920v2", "C920V2", "XuanTie", UNKN_STR, PURPOSE_PERFORMANCE },
	{ 0, NULL,           NULL,     NULL,      NULL,     PURPOSE_GENERAL     },
};

static const struct riscv_part_t andes_part[] = {
	{ 0, "andestech,ax45mp", "AX45MP", UNKN_STR, UNKN_STR, PURPOSE_GENERAL },
	{ 0, NULL,               NULL,     NULL,     NULL,     PURPOSE_GENERAL },
};

static const struct riscv_part_t mips_part[] = {
	{ 0, "mips,p8700", "P8700", UNKN_STR, UNKN_STR, PURPOSE_PERFORMANCE },
	{ 0, NULL,         NULL,    NULL,     NULL,     PURPOSE_GENERAL     },
};

/* Purposes: the K3 SoC mixes X100 compute cores with A100 "AI" cores,
   they get distinct purposes so that cpu_identify_all() reports them separately */
static const struct riscv_part_t spacemit_part[] = {
	{ 0x8000000058000001ULL, "spacemit,x60",  "X60",  UNKN_STR, "12 nm",  PURPOSE_GENERAL     },
	{ 0x8000000058000002ULL, "spacemit,x100", "X100", UNKN_STR, UNKN_STR, PURPOSE_PERFORMANCE },
	{ 0x8000000041000002ULL, "spacemit,a100", "A100", UNKN_STR, UNKN_STR, PURPOSE_GENERAL     },
	{ 0,                     NULL,            NULL,   NULL,     NULL,     PURPOSE_GENERAL     },
};

static const struct riscv_part_t unknown_part[] = {
	{ 0, NULL, NULL, NULL, NULL, PURPOSE_GENERAL },
};

/* Vendors list
   IDs from Linux (arch/riscv/include/asm/vendorid_list.h) and LLVM (RISCVProcessors.td) */
static const struct riscv_vendor_t hw_vendor[] = {
	{ 0x000, VENDOR_UNKNOWN,   open_source_part, UNKN_STR    },
	{ 0x029, VENDOR_MICROCHIP, unknown_part,     "Microchip" },
	{ 0x127, VENDOR_MIPS,      mips_part,        "MIPS"      },
	{ 0x31e, VENDOR_ANDES,     andes_part,       "Andes"     },
	{ 0x489, VENDOR_SIFIVE,    sifive_part,      "SiFive"    },
	{ 0x5b7, VENDOR_THEAD,     thead_part,       "T-Head"    },
	{ 0x710, VENDOR_SPACEMIT,  spacemit_part,    "SpacemiT"  },
	{ -1,    VENDOR_UNKNOWN,   unknown_part,     UNKN_STR    },
};

/* RISC-V ISA extensions, as reported by the Linux hwprobe syscall
   (RISCV_HWPROBE_KEY_IMA_EXT_0 and RISCV_HWPROBE_KEY_IMA_EXT_1 in arch/riscv/include/uapi/asm/hwprobe.h) */
struct riscv_hwprobe_feature_t {
	const uint8_t reg; /* index in cpu_raw_data_t::riscv_ima_ext */
	const uint8_t bit;
	const cpu_feature_t feature;
};

static const struct riscv_hwprobe_feature_t hwprobe_feature[] = {
	/* RISCV_HWPROBE_KEY_IMA_EXT_0 */
	{ 0,  0, CPU_FEATURE_RV_F }, /* RISCV_HWPROBE_IMA_FD */
	{ 0,  0, CPU_FEATURE_RV_D }, /* RISCV_HWPROBE_IMA_FD */
	{ 0,  1, CPU_FEATURE_RV_C },
	{ 0,  2, CPU_FEATURE_RV_V },
	{ 0,  3, CPU_FEATURE_ZBA },
	{ 0,  4, CPU_FEATURE_ZBB },
	{ 0,  5, CPU_FEATURE_ZBS },
	{ 0,  6, CPU_FEATURE_ZICBOZ },
	{ 0,  7, CPU_FEATURE_ZBC },
	{ 0,  8, CPU_FEATURE_ZBKB },
	{ 0,  9, CPU_FEATURE_ZBKC },
	{ 0, 10, CPU_FEATURE_ZBKX },
	{ 0, 11, CPU_FEATURE_ZKND },
	{ 0, 12, CPU_FEATURE_ZKNE },
	{ 0, 13, CPU_FEATURE_ZKNH },
	{ 0, 14, CPU_FEATURE_ZKSED },
	{ 0, 15, CPU_FEATURE_ZKSH },
	{ 0, 16, CPU_FEATURE_ZKT },
	{ 0, 17, CPU_FEATURE_ZVBB },
	{ 0, 18, CPU_FEATURE_ZVBC },
	{ 0, 19, CPU_FEATURE_ZVKB },
	{ 0, 20, CPU_FEATURE_ZVKG },
	{ 0, 21, CPU_FEATURE_ZVKNED },
	{ 0, 22, CPU_FEATURE_ZVKNHA },
	{ 0, 23, CPU_FEATURE_ZVKNHB },
	{ 0, 24, CPU_FEATURE_ZVKSED },
	{ 0, 25, CPU_FEATURE_ZVKSH },
	{ 0, 26, CPU_FEATURE_ZVKT },
	{ 0, 27, CPU_FEATURE_ZFH },
	{ 0, 28, CPU_FEATURE_ZFHMIN },
	{ 0, 29, CPU_FEATURE_ZIHINTNTL },
	{ 0, 30, CPU_FEATURE_ZVFH },
	{ 0, 31, CPU_FEATURE_ZVFHMIN },
	{ 0, 32, CPU_FEATURE_ZFA },
	{ 0, 33, CPU_FEATURE_ZTSO },
	{ 0, 34, CPU_FEATURE_ZACAS },
	{ 0, 35, CPU_FEATURE_ZICOND },
	{ 0, 36, CPU_FEATURE_ZIHINTPAUSE },
	{ 0, 37, CPU_FEATURE_ZVE32X },
	{ 0, 38, CPU_FEATURE_ZVE32F },
	{ 0, 39, CPU_FEATURE_ZVE64X },
	{ 0, 40, CPU_FEATURE_ZVE64F },
	{ 0, 41, CPU_FEATURE_ZVE64D },
	{ 0, 42, CPU_FEATURE_ZIMOP },
	{ 0, 43, CPU_FEATURE_ZCA },
	{ 0, 44, CPU_FEATURE_ZCB },
	{ 0, 45, CPU_FEATURE_ZCD },
	{ 0, 46, CPU_FEATURE_ZCF },
	{ 0, 47, CPU_FEATURE_ZCMOP },
	{ 0, 48, CPU_FEATURE_ZAWRS },
	{ 0, 49, CPU_FEATURE_SUPM },
	{ 0, 50, CPU_FEATURE_ZICNTR },
	{ 0, 51, CPU_FEATURE_ZIHPM },
	{ 0, 52, CPU_FEATURE_ZFBFMIN },
	{ 0, 53, CPU_FEATURE_ZVFBFMIN },
	{ 0, 54, CPU_FEATURE_ZVFBFWMA },
	{ 0, 55, CPU_FEATURE_ZICBOM },
	{ 0, 56, CPU_FEATURE_ZAAMO },
	{ 0, 57, CPU_FEATURE_ZALRSC },
	{ 0, 58, CPU_FEATURE_ZABHA },
	{ 0, 59, CPU_FEATURE_ZALASR },
	{ 0, 60, CPU_FEATURE_ZICBOP },
	{ 0, 61, CPU_FEATURE_ZILSD },
	{ 0, 62, CPU_FEATURE_ZCLSD },
	{ 0, 63, CPU_FEATURE_ZICFILP },
	/* RISCV_HWPROBE_KEY_IMA_EXT_1 */
	{ 1,  0, CPU_FEATURE_ZICFISS },
	{ 1,  1, CPU_FEATURE_ZICCLSM },
	{ 1,  2, CPU_FEATURE_ZICCAMOA },
	{ 1,  3, CPU_FEATURE_ZICCIF },
	{ 1,  4, CPU_FEATURE_ZICCRSE },
	{ 1,  5, CPU_FEATURE_ZA64RS },
};

/* Mandatory extensions of the RVA profiles (user-mode part), limited to the ones hwprobe can report
   https://github.com/riscv/riscv-profiles/blob/main/src/profiles.adoc
   Note: the kernel must report an extension for it to count, Linux 6.13 reports all of them */
static const cpu_feature_t rva20u64_mandatory[] = {
	CPU_FEATURE_RV_I, CPU_FEATURE_RV_M, CPU_FEATURE_RV_A, CPU_FEATURE_RV_F, CPU_FEATURE_RV_D, CPU_FEATURE_RV_C,
	NUM_CPU_FEATURES
};
static const cpu_feature_t rva22u64_mandatory[] = {
	CPU_FEATURE_ZBA, CPU_FEATURE_ZBB, CPU_FEATURE_ZBS, CPU_FEATURE_ZICBOZ, CPU_FEATURE_ZICBOM, CPU_FEATURE_ZICBOP,
	CPU_FEATURE_ZFHMIN, CPU_FEATURE_ZKT, CPU_FEATURE_ZIHINTPAUSE, CPU_FEATURE_ZICNTR, CPU_FEATURE_ZIHPM,
	NUM_CPU_FEATURES
};
static const cpu_feature_t rva23u64_mandatory[] = {
	CPU_FEATURE_RV_V, CPU_FEATURE_ZVFHMIN, CPU_FEATURE_ZVBB, CPU_FEATURE_ZVKT, CPU_FEATURE_ZIHINTNTL, CPU_FEATURE_ZICOND,
	CPU_FEATURE_ZIMOP, CPU_FEATURE_ZCMOP, CPU_FEATURE_ZCB, CPU_FEATURE_ZFA, CPU_FEATURE_ZAWRS, CPU_FEATURE_SUPM,
	NUM_CPU_FEATURES
};

static const struct riscv_vendor_t* get_cpu_vendor_from_id(uint64_t mvendorid)
{
	int i;

	for (i = 0; hw_vendor[i].mvendorid >= 0; i++) {
		if ((uint64_t) hw_vendor[i].mvendorid == mvendorid)
			break;
	}

	/* This function returns an item with VENDOR_UNKNOWN if mvendorid is not found in the "hw_vendor" array */
	return &hw_vendor[i];
}

static const struct riscv_vendor_t* get_cpu_vendor_from_vendor(cpu_vendor_t vendor)
{
	int i;

	/* Skip the first item (open-source cores, mvendorid 0) which also uses VENDOR_UNKNOWN */
	for (i = 1; hw_vendor[i].mvendorid >= 0; i++) {
		if (hw_vendor[i].vendor == vendor)
			break;
	}

	return &hw_vendor[i];
}

static const struct riscv_part_t* get_cpu_vendor_parts(const struct riscv_vendor_t* hw_vendor, struct cpu_raw_data_t* raw)
{
	int i;

	for (i = 0; hw_vendor->parts[i].name != NULL; i++) {
		if ((hw_vendor->parts[i].marchid != 0) && (hw_vendor->parts[i].marchid == raw->riscv_marchid))
			break;
		if ((hw_vendor->parts[i].uarch != NULL) && (raw->riscv_uarch[0] != '\0') && !strcmp(hw_vendor->parts[i].uarch, raw->riscv_uarch))
			break;
	}

	/* This function returns the terminator item (name is NULL) if the core is not found in the "parts" array */
	return &hw_vendor->parts[i];
}

static void load_riscv_features(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	unsigned i;

	/* RISCV_HWPROBE_KEY_BASE_BEHAVIOR */
	if (raw->riscv_base_behavior & 0x1) { /* RISCV_HWPROBE_BASE_BEHAVIOR_IMA */
		data->flags[CPU_FEATURE_RV_I] = 1;
		data->flags[CPU_FEATURE_RV_M] = 1;
		data->flags[CPU_FEATURE_RV_A] = 1;
	}

	for (i = 0; i < COUNT_OF(hwprobe_feature); i++) {
		if (hwprobe_feature[i].reg >= MAX_RISCV_IMA_EXT_REGS)
			continue;
		if (raw->riscv_ima_ext[hwprobe_feature[i].reg] & (1ULL << hwprobe_feature[i].bit))
			data->flags[hwprobe_feature[i].feature] = 1;
	}
}

static bool has_all_features(const struct cpu_id_t* data, const cpu_feature_t* features)
{
	int i;

	for (i = 0; features[i] != NUM_CPU_FEATURES; i++)
		if (!data->flags[features[i]])
			return false;

	return true;
}

static void decode_riscv_profile(struct cpu_id_t* data)
{
	data->feature_level = FEATURE_LEVEL_UNKNOWN;
	if (!has_all_features(data, rva20u64_mandatory))
		return;
	data->feature_level = FEATURE_LEVEL_RISCV_RVA20U64;
	if (!has_all_features(data, rva22u64_mandatory))
		return;
	data->feature_level = FEATURE_LEVEL_RISCV_RVA22U64;
	if (!has_all_features(data, rva23u64_mandatory))
		return;
	data->feature_level = FEATURE_LEVEL_RISCV_RVA23U64;
}

int cpuid_identify_riscv(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	const char* uarch_name;

	/* Basic values extraction
	   Based on "The RISC-V Instruction Set Manual: Volume II: Privileged Architecture",
	   sections "Machine Vendor ID Register", "Machine Architecture ID Register" and "Machine Implementation ID Register"
	*/
	data->riscv.mvendorid = raw->riscv_mvendorid;
	data->riscv.marchid   = raw->riscv_marchid;
	data->riscv.mimpid    = raw->riscv_mimpid;
	data->riscv.vlen      = (raw->riscv_vlenb > 0) ? (int32_t) (raw->riscv_vlenb * 8) : -1;
	data->purpose         = cpuid_identify_purpose_riscv(raw);

	/* Values decoding */
	const struct riscv_vendor_t* hw_vendor = get_cpu_vendor_from_id(raw->riscv_mvendorid);
	const struct riscv_part_t* part = get_cpu_vendor_parts(hw_vendor, raw);
	data->vendor = hw_vendor->vendor;
	snprintf(data->vendor_str, VENDOR_STR_MAX, "%s", hw_vendor->name);
	if (part->name != NULL) {
		snprintf(data->brand_str,       BRAND_STR_MAX,      "%s", part->name);
		snprintf(data->cpu_codename,    CODENAME_STR_MAX,   "%s", part->codename);
		snprintf(data->technology_node, TECHNOLOGY_STR_MAX, "%s", part->technology);
	}
	else {
		/* Unknown core: fallback to the device tree name, without the "vendor," prefix */
		uarch_name = strchr(raw->riscv_uarch, ',');
		uarch_name = (uarch_name != NULL) ? uarch_name + 1 : raw->riscv_uarch;
		snprintf(data->brand_str,       BRAND_STR_MAX,      "%s", (uarch_name[0] != '\0') ? uarch_name : UNKN_STR);
		snprintf(data->cpu_codename,    CODENAME_STR_MAX,   "%s", UNKN_STR);
		snprintf(data->technology_node, TECHNOLOGY_STR_MAX, "%s", UNKN_STR);
	}
	load_riscv_features(raw, data);
	decode_riscv_profile(data);

	return 0;
}

void cpuid_get_list_riscv(cpu_vendor_t vendor, struct cpu_list_t* list)
{
	int i, j, n, good, sub_count, total_count;
	char **tmp_names = NULL;
	const struct riscv_vendor_t* hw_vendor = get_cpu_vendor_from_vendor(vendor);

	/* Get item count in hw_vendor->parts */
	sub_count = 0;
	for (i = 0; hw_vendor->parts[i].name != NULL; i++)
		sub_count++;
	total_count = list->num_entries + sub_count;
	if (sub_count == 0)
		return;

	/* Grow current list */
	n = list->num_entries;
	tmp_names = (char**) realloc(list->names, sizeof(char*) * total_count);
	if (!tmp_names) { /* Memory allocation failure */
		cpuid_set_error(ERR_NO_MEM);
		return;
	}
	list->names = tmp_names;

	/* Append item to list */
	for (i = 0; i < sub_count; i++) {
		good = 1;
		for (j = n - 1; j >= 0; j--)
			if (!strcmp(list->names[j], hw_vendor->parts[i].name)) {
				good = 0;
				break;
			}
		if (!good) continue;
#if defined(_MSC_VER)
		list->names[n] = _strdup(hw_vendor->parts[i].name);
#else
		list->names[n] = strdup(hw_vendor->parts[i].name);
#endif
		if (!list->names[n]) { /* Memory allocation failure */
			cpuid_set_error(ERR_NO_MEM);
			list->num_entries = 0;
			for (j = 0; j < n; j++) {
				free(list->names[j]);
			}
			free(list->names);
			list->names = NULL;
			return;
		}
		n++;
	}
	list->num_entries = n;
}

cpu_purpose_t cpuid_identify_purpose_riscv(struct cpu_raw_data_t* raw)
{
	const struct riscv_vendor_t* hw_vendor = get_cpu_vendor_from_id(raw->riscv_mvendorid);
	const struct riscv_part_t* part = get_cpu_vendor_parts(hw_vendor, raw);

	return (part->name != NULL) ? part->purpose : PURPOSE_GENERAL;
}
