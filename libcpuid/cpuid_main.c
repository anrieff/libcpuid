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
#include "libcpuid.h"
#include "recog_intel.h"
#include "recog_amd.h"
#include "asm-bits.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>

#define COUNT_OF(array) (sizeof(array) / sizeof(array[0]))

/* Implementation: */

static int _libcpiud_errno = ERR_OK;

static void default_warn(const char *msg)
{
	printf("%s", msg);
}

static void (*_warn) (const char* msg) = default_warn;

static int set_error(cpuid_error_t err)
{
	_libcpiud_errno = (int) err;
	return (int) err;
}

static void raw_data_t_constructor(struct cpu_raw_data_t* raw)
{
	memset(raw, 0, sizeof(struct cpu_raw_data_t));
}

static void cpu_id_t_constructor(struct cpu_id_t* id)
{
	memset(id, 0, sizeof(struct cpu_id_t));
	id->l1_data_cache = id->l1_instruction_cache = id->l2_cache = id->l3_cache = -1;
	id->l1_assoc = id->l2_assoc = id->l3_assoc = -1;
}

static int parse_token(const char* expected_token, const char *token,
                        const char *value, uint32_t array[][4], int limit, int *recognized)
{
	char format[32];
	int veax, vebx, vecx, vedx;
	int index;

	if (*recognized) return 1; // already recognized
	if (strncmp(token, expected_token, strlen(expected_token))) return 1; // not what we search for
	sprintf(format, "%s[%%d]", expected_token);
	*recognized = 1;
	if (1 == sscanf(token, format, &index) && index >=0 && index < limit) {
		if (4 == sscanf(value, "%x%x%x%x", &veax, &vebx, &vecx, &vedx)) {
			array[index][0] = veax;
			array[index][1] = vebx;
			array[index][2] = vecx;
			array[index][3] = vedx;
			return 1;
		}
	}
	return 0;
}

struct feature_map_t {
	unsigned bit;
	cpu_feature_t feature;
};
 
static void match_features(const struct feature_map_t* matchtable, int count, uint32_t reg, struct cpu_id_t* data)
{
	int i;
	for (i = 0; i < count; i++)
		if (reg & (1 << matchtable[i].bit))
			data->flags[matchtable[i].feature] = 1;
}

static void load_features_common(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	const struct feature_map_t matchtable_edx1[] = {
		{  0, CPU_FEATURE_FPU },
		{  1, CPU_FEATURE_VME },
		{  2, CPU_FEATURE_DEBUG },
		{  3, CPU_FEATURE_PSE },
		{  4, CPU_FEATURE_TSC },
		{  5, CPU_FEATURE_MSR },
		{  6, CPU_FEATURE_PAE },
		{  7, CPU_FEATURE_MCE },
		{  8, CPU_FEATURE_CX8 },
		{  9, CPU_FEATURE_APIC },
		{ 12, CPU_FEATURE_MTRR },
		{ 13, CPU_FEATURE_PGE },
		{ 14, CPU_FEATURE_MCA },
		{ 15, CPU_FEATURE_CMOV },
		{ 16, CPU_FEATURE_PAT },
		{ 17, CPU_FEATURE_PSE36 },
		{ 19, CPU_FEATURE_CLFLUSH },
		{ 23, CPU_FEATURE_MMX },
		{ 24, CPU_FEATURE_FXSR },
		{ 25, CPU_FEATURE_SSE },
		{ 26, CPU_FEATURE_SSE2 },
		{ 28, CPU_FEATURE_HT },
	};
	const struct feature_map_t matchtable_ecx1[] = {
		{  0, CPU_FEATURE_SSE3 },
		{  3, CPU_FEATURE_MON },
		{  9, CPU_FEATURE_SSSE3 },
		{ 13, CPU_FEATURE_CX16 },
		{ 19, CPU_FEATURE_SSE41 },
	};
	const struct feature_map_t matchtable_edx81[] = {
		{ 29, CPU_FEATURE_LM },
	};
	const struct feature_map_t matchtable_ecx81[] = {
		{ 0, CPU_FEATURE_LAHFSAHF },
	};
	if (raw->basic_cpuid[0][0] >= 1) {
		match_features(matchtable_edx1, COUNT_OF(matchtable_edx1), raw->basic_cpuid[1][3], data);
		match_features(matchtable_ecx1, COUNT_OF(matchtable_ecx1), raw->basic_cpuid[1][2], data);
	}
	if (raw->ext_cpuid[0][0] >= 1) {
		match_features(matchtable_edx81, COUNT_OF(matchtable_edx81), raw->ext_cpuid[1][3], data);
		match_features(matchtable_ecx81, COUNT_OF(matchtable_ecx81), raw->ext_cpuid[1][2], data);
	}
}

static int cpuid_basic_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int i, j;
	
	memcpy(data->vendor_str + 0, &raw->basic_cpuid[0][1], 4);
	memcpy(data->vendor_str + 4, &raw->basic_cpuid[0][3], 4);
	memcpy(data->vendor_str + 8, &raw->basic_cpuid[0][2], 4);
	data->vendor_str[12] = 0;
	/* Determine vendor: */
	const struct { cpu_vendor_t vendor; char match[16]; }
	matchtable[NUM_VENDORS] = {
		/* source: http://www.sandpile.org/ia32/cpuid.htm */
		{ INTEL		, "GenuineIntel" },
		{ AMD		, "AuthenticAMD" },
		{ CYRIX		, "CyrixInstead" },
		{ NEXGEN	, "NexGenDriven" },
		{ TRANSMETA	, "GenuineTMx86" },
		{ UMC		, "UMC UMC UMC " },
		{ CENTAUR	, "CentaurHauls" },
		{ RISE		, "RiseRiseRise" },
		{ SIS		, "SiS SiS SiS " },
		{ NSC		, "Geode by NSC" },
	};
	data->vendor = UNKNOWN;
	for (i = 0; i < NUM_VENDORS; i++)
		if (!strcmp(data->vendor_str, matchtable[i].match)) {
			data->vendor = matchtable[i].vendor;
			break;
		}
	if (data->vendor == UNKNOWN)
		return set_error(ERR_CPU_UNKN);
	int basic = raw->basic_cpuid[0][0];
	if (basic >= 1) {
		data->family = (raw->basic_cpuid[1][0] >> 8) & 0xf;
		data->model = (raw->basic_cpuid[1][0] >> 4) & 0xf;
		data->stepping = raw->basic_cpuid[1][0] & 0xf;
		data->ext_model = (raw->basic_cpuid[1][0] >> 16) & 0xf;
		data->ext_family = (raw->basic_cpuid[1][0] >> 20) & 0xff;
	}
	int ext = raw->ext_cpuid[0][0] - 0x8000000;
	
	/* obtain the brand string, if present: */
	char brandstr[64] = {0};
	if (ext >= 4) {
		for (i = 0; i < 3; i++)
			for (j = 0; j < 4; j++)
				memcpy(brandstr + i * 16 + j * 4,
				       &raw->ext_cpuid[2 + i][j], 4);
		brandstr[48] = 0;
		i = 0;
		while (brandstr[i] == ' ') i++;
		strncpy(data->brand_str, brandstr + i, sizeof(data->brand_str));
		data->brand_str[48] = 0;
	}
	load_features_common(raw, data);
	return set_error(ERR_OK);
}


/* Interface: */

int cpuid_present(void)
{
	return cpuid_exists_by_eflags();
}

void cpu_exec_cpuid(uint32_t eax, uint32_t* regs)
{
	regs[0] = eax;
	regs[1] = regs[2] = regs[3] = 0;
	exec_cpiud(regs);
}

void cpu_exec_cpuid_ext(uint32_t* regs)
{
	exec_cpiud(regs);
}

int cpuid_get_raw_data(struct cpu_raw_data_t* data)
{
	unsigned i;
	if (!cpuid_present())
		return set_error(ERR_NO_CPUID);
	for (i = 0; i < 32; i++)
		cpu_exec_cpuid(i, data->basic_cpuid[i]);
	for (i = 0; i < 32; i++)
		cpu_exec_cpuid(0x80000000 + i, data->ext_cpuid[i]);
	for (i = 0; i < 4; i++) {
		memset(data->intel_fn4[i], 0, sizeof(data->intel_fn4[i]));
		data->intel_fn4[i][0] = 4;
		data->intel_fn4[i][2] = i;
		cpu_exec_cpuid_ext(data->intel_fn4[i]);
	}
	return set_error(ERR_OK);
}

int cpuid_serialize_raw_data(struct cpu_raw_data_t* data, const char* filename)
{
	int i;
	FILE *f;
	
	f = fopen(filename, "wt");
	if (!f) return set_error(ERR_OPEN);
	
	fprintf(f, "version=%s\n", VERSION);
	for (i = 0; i < MAX_CPUID_LEVEL; i++)
		fprintf(f, "basic_cpuid[%d]=%08x %08x %08x %08x\n", i,
			data->basic_cpuid[i][0], data->basic_cpuid[i][1],
			data->basic_cpuid[i][2], data->basic_cpuid[i][3]);
	for (i = 0; i < MAX_EXT_CPUID_LEVEL; i++)
		fprintf(f, "ext_cpuid[%d]=%08x %08x %08x %08x\n", i,
			data->ext_cpuid[i][0], data->ext_cpuid[i][1],
			data->ext_cpuid[i][2], data->ext_cpuid[i][3]);
	for (i = 0; i < MAX_INTELFN4_LEVEL; i++)
		fprintf(f, "intel_fn4[%d]=%08x %08x %08x %08x\n", i,
			data->intel_fn4[i][0], data->intel_fn4[i][1],
			data->intel_fn4[i][2], data->intel_fn4[i][3]);
	
	fclose(f);
	return set_error(ERR_OK);
}

int cpuid_deserialize_raw_data(struct cpu_raw_data_t* data, const char* filename)
{
	int i, len;
	char temp[100];
	char line[100];
	char token[100];
	char *value;
	int syntax;
	int cur_line = 0;
	int recognized;
	FILE *f;
	
	raw_data_t_constructor(data);
	
	f = fopen(filename, "rt");
	if (!f) return set_error(ERR_OPEN);
	while (fgets(line, sizeof(line), f)) {
		++cur_line;
		len = strlen(line);
		if (len < 2) continue;
		if (line[len - 1] == '\n')
			line[--len] = '\0';
		for (i = 0; i < len && line[i] != '='; i++)
		if (i >= len && i < 1 && len - i - 1 <= 0) {
			fclose(f);
			return set_error(ERR_BADFMT);
		}
		strncpy(token, line, i);
		token[i] = '\0';
		value = &line[i + 1];
		// try to recognize the line
		recognized = 0;
		if (!strcmp(token, "version")) {
			recognized = 1;
		}
		syntax = 1;
		syntax = syntax && parse_token("basic_cpuid", token, value, data->basic_cpuid, 32, &recognized);
		syntax = syntax && parse_token("ext_cpuid", token, value, data->ext_cpuid, 32, &recognized);
		syntax = syntax && parse_token("intel_fn4", token, value, data->intel_fn4,  4, &recognized);
		if (!syntax) {
			sprintf(temp, "Error: %s:%d: Syntax error\n",
				filename, cur_line);
			_warn(temp);
			fclose(f);
			return set_error(ERR_BADFMT);
		}
		if (!recognized) {
			sprintf(temp, "Warning: %s:%d not understood!\n",
			        filename, cur_line);
			_warn(temp);
		}
	}
	
	fclose(f);
	return set_error(ERR_OK);
}

int cpu_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int r;
	struct cpu_raw_data_t myraw;
	if (!raw) {
		cpuid_get_raw_data(&myraw);
		raw = &myraw;
	}
	cpu_id_t_constructor(data);
	if ((r = cpuid_basic_identify(raw, data)) < 0)
		return set_error(r);
	switch (data->vendor) {
		case INTEL:
			r = cpuid_identify_intel(raw, data);
			break;
		case AMD:
			r = cpuid_identify_amd(raw, data);
			break;
		default:
			break;
	}
	return set_error(r);
}

const char* cpu_feature_str(cpu_feature_t feature)
{
	const struct { cpu_feature_t feature; const char* name; }
	matchtable[] = {
		{ CPU_FEATURE_FPU, "fpu" },
		{ CPU_FEATURE_VME, "vme" },
		{ CPU_FEATURE_DEBUG, "debug" },
		{ CPU_FEATURE_PSE, "pse" },
		{ CPU_FEATURE_TSC, "tsc" },
		{ CPU_FEATURE_MSR, "msr" },
		{ CPU_FEATURE_PAE, "pae" },
		{ CPU_FEATURE_MCE, "mce" },
		{ CPU_FEATURE_CX8, "cx8" },
		{ CPU_FEATURE_APIC, "apic" },
		{ CPU_FEATURE_MTRR, "mtrr" },
		{ CPU_FEATURE_PGE, "pge" },
		{ CPU_FEATURE_MCA, "mca" },
		{ CPU_FEATURE_CMOV, "cmov" },
		{ CPU_FEATURE_PAT, "pat" },
		{ CPU_FEATURE_PSE36, "pse36" },
		{ CPU_FEATURE_PSN, "psn" },
		{ CPU_FEATURE_CLFLUSH, "clflush" },
		{ CPU_FEATURE_DTS, "dts" },
		{ CPU_FEATURE_ACPI, "acpi" },
		{ CPU_FEATURE_MMX, "mmx" },
		{ CPU_FEATURE_FXSR, "fxsr" },
		{ CPU_FEATURE_SSE, "sse" },
		{ CPU_FEATURE_SSE2, "sse2" },
		{ CPU_FEATURE_SS, "ss" },
		{ CPU_FEATURE_HT, "ht" },
		{ CPU_FEATURE_TM, "tm" },
		{ CPU_FEATURE_IA64, "ia64" },
		{ CPU_FEATURE_PBE, "pbe" },
		{ CPU_FEATURE_SSE3, "sse3" },
		{ CPU_FEATURE_PCLMUL, "pclmul" },
		{ CPU_FEATURE_DTS64, "dts64" },
		{ CPU_FEATURE_MON, "mon" },
		{ CPU_FEATURE_DSCPL, "dscpl" },
		{ CPU_FEATURE_VMX, "vmx" },
		{ CPU_FEATURE_SMX, "smx" },
		{ CPU_FEATURE_EST, "est" },
		{ CPU_FEATURE_TM2, "tm2" },
		{ CPU_FEATURE_SSSE3, "ssse3" },
		{ CPU_FEATURE_CID, "cid" },
		{ CPU_FEATURE_CX16, "cx16" },
		{ CPU_FEATURE_ETPRD, "etprd" },
		{ CPU_FEATURE_PDCM, "pdcm" },
		{ CPU_FEATURE_DCA, "dca" },
		{ CPU_FEATURE_SSE41, "sse41" },
		{ CPU_FEATURE_SSE42, "sse42" },
		{ CPU_FEATURE_MOVBE, "movbe" },
		{ CPU_FEATURE_POPCNT, "popcnt" },
		{ CPU_FEATURE_AES, "aes" },
		{ CPU_FEATURE_XSAVE, "xsave" },
		{ CPU_FEATURE_OSXSAVE, "osxsave" },
		{ CPU_FEATURE_AVX, "avx" },
		{ CPU_FEATURE_MMXEXT, "mmxext" },
		{ CPU_FEATURE_3DNOW, "3dnow" },
		{ CPU_FEATURE_3DNOWEXT, "3dnowext" },
		{ CPU_FEATURE_NX, "nx" },
		{ CPU_FEATURE_RDTSCP, "rdtscp" },
		{ CPU_FEATURE_LM, "lm" },
		{ CPU_FEATURE_LAHFSAHF, "lahfsahf" },
		{ CPU_FEATURE_SVM, "svm" },
		{ CPU_FEATURE_LZCNT, "lzcnt" },
		{ CPU_FEATURE_3DNOWPREFETCH, "3dnowprefetch" },
		{ CPU_FEATURE_OSVW, "osvw" },
		{ CPU_FEATURE_IBS, "ibs" },
		{ CPU_FEATURE_SSE51, "sse51" },
		{ CPU_FEATURE_SKINIT, "skinit" },
		{ CPU_FEATURE_WDT, "wdt" },
		{ CPU_FEATURE_CONST_TSC, "const_tsc" },
	};
	unsigned i;
	for (i = 0; i < COUNT_OF(matchtable); i++)
		if (matchtable[i].feature == feature)
			return matchtable[i].name;
	return "";
}

const char* cpuid_error(void)
{
	const struct { cpuid_error_t error; const char *description; }
	matchtable[] = {
		{ ERR_OK       , "No error"},
		{ ERR_NO_CPUID , "CPUID instruction is not supported"},
		{ ERR_NO_RDTSC , "RDTSC instruction is not supported"},
		{ ERR_NO_MEM   , "Memory allocation failed"},
		{ ERR_OPEN     , "File open operation failed"},
		{ ERR_BADFMT   , "Bad file format"},
		{ ERR_NOT_IMP  , "Not implemented"},
		{ ERR_CPU_UNKN , "Unsupported processor"},
	};
	unsigned i;
	for (i = 0; i < COUNT_OF(matchtable); i++)
		if (_libcpiud_errno == matchtable[i].error)
			return matchtable[i].description;
	return "Unknown error";
}


const char* cpuid_lib_version(void)
{
	return VERSION;
}

void set_warn_function(void (*warn_fun) (const char* msg))
{
	_warn = warn_fun;
}

