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
#include "libcpuid_internal.h"
#include "recog_intel.h"
#include "recog_amd.h"
#include "asm-bits.h"
#include "libcpuid_util.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/* Implementation: */

#if defined(__STDC_VERSION__) && __STDC_VERSION__ == 201112L
	_Thread_local int _libcpiud_errno = ERR_OK;
#elif defined(__GNUC__) // Also works for clang
	__thread int _libcpiud_errno = ERR_OK;
#else
	static int _libcpiud_errno = ERR_OK;
#endif

int set_error(cpu_error_t err)
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
	id->l1_data_cache = id->l1_instruction_cache = id->l2_cache = id->l3_cache = id->l4_cache = -1;
	id->l1_assoc = id->l1_data_assoc = id->l1_instruction_assoc = id->l2_assoc = id->l3_assoc = id->l4_assoc = -1;
	id->l1_cacheline = id->l1_data_cacheline = id->l1_instruction_cacheline = id->l2_cacheline = id->l3_cacheline = id->l4_cacheline = -1;
	id->sse_size = -1;
	id->affinity_mask = 0x00000000;
	id->purpose = PURPOSE_GENERAL;
}

/* get_total_cpus() system specific code: uses OS routines to determine total number of CPUs */
#ifdef __APPLE__
#include <unistd.h>
#include <mach/clock_types.h>
#include <mach/clock.h>
#include <mach/mach.h>
#include <mach/thread_policy.h>
static int get_total_cpus(void)
{
	kern_return_t kr;
	host_basic_info_data_t basic_info;
	host_info_t info = (host_info_t)&basic_info;
	host_flavor_t flavor = HOST_BASIC_INFO;
	mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
	kr = host_info(mach_host_self(), flavor, info, &count);
	if (kr != KERN_SUCCESS) return 1;
	return basic_info.avail_cpus;
}
#define GET_TOTAL_CPUS_DEFINED

static int set_cpu_affinity(uint32_t logical_cpu)
{
	thread_affinity_policy_data_t ap;
	ap.affinity_tag = logical_cpu + 1;
	return thread_policy_set(mach_thread_self(), THREAD_AFFINITY_POLICY, (thread_policy_t) &ap, THREAD_AFFINITY_POLICY_COUNT) == KERN_SUCCESS;
}
#define SET_CPU_AFFINITY
#endif /* __APPLE__ */

#ifdef _WIN32
#include <windows.h>
static int get_total_cpus(void)
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return system_info.dwNumberOfProcessors;
}
#define GET_TOTAL_CPUS_DEFINED

static int set_cpu_affinity(uint32_t logical_cpu)
{
	HANDLE process = GetCurrentProcess();
	DWORD_PTR processAffinityMask = 1ULL << logical_cpu;
	return !SetProcessAffinityMask(process, processAffinityMask);
}
#define SET_CPU_AFFINITY
#endif /* _WIN32 */

#ifdef __HAIKU__
#include <OS.h>
static int get_total_cpus(void)
{
	system_info info;
	get_system_info(&info);
	return info.cpu_count;
}
#define GET_TOTAL_CPUS_DEFINED
#endif /* __HAIKU__ */

#if defined linux || defined __linux__ || defined __sun
#include <sys/sysinfo.h>
#include <unistd.h>

static int get_total_cpus(void)
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}
#define GET_TOTAL_CPUS_DEFINED
#endif /* defined linux || defined __linux__ || defined __sun */

#if defined linux || defined __linux__
#include <sched.h>

static int set_cpu_affinity(uint32_t logical_cpu)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(logical_cpu, &cpuset);
	return sched_setaffinity(0, sizeof(cpuset), &cpuset);
}
#define SET_CPU_AFFINITY
#endif /* defined linux || defined __linux__ */

#if defined sun || defined __sun
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>

static int set_cpu_affinity(uint32_t logical_cpu)
{
	return processor_bind(P_LWPID, P_MYID, logical_cpu, NULL) == 0;
}
#define SET_CPU_AFFINITY
#endif /* defined sun || defined __sun */

#if defined __FreeBSD__ || defined __OpenBSD__ || defined __NetBSD__ || defined __bsdi__ || defined __QNX__
#include <sys/types.h>
#include <sys/sysctl.h>

static int get_total_cpus(void)
{
	int mib[2] = { CTL_HW, HW_NCPU };
	int ncpus;
	size_t len = sizeof(ncpus);
	if (sysctl(mib, 2, &ncpus, &len, (void *) 0, 0) != 0) return 1;
	return ncpus;
}
#define GET_TOTAL_CPUS_DEFINED
#endif /* defined __FreeBSD__ || defined __OpenBSD__ || defined __NetBSD__ || defined __bsdi__ || defined __QNX__ */

#if defined __FreeBSD__
#include <sys/param.h>
#include <sys/cpuset.h>

static int set_cpu_affinity(uint32_t logical_cpu)
{
	cpuset_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(logical_cpu, &cpuset);
	return cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, sizeof(cpuset), &cpuset);
}
#define SET_CPU_AFFINITY
#endif /* defined __FreeBSD__ */

#if defined __DragonFly__
#include <pthread.h>
#include <pthread_np.h>

static int set_cpu_affinity(uint32_t logical_cpu)
{
	cpuset_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(logical_cpu, &cpuset);
	return pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
}
#define SET_CPU_AFFINITY
#endif /* defined __DragonFly__ */

#if defined __NetBSD__
#include <pthread.h>
#include <sched.h>

static int set_cpu_affinity(uint32_t logical_cpu)
{
	cpuset_t *cpuset;
	cpuset = cpuset_create();
	cpuset_set((cpuid_t) logical_cpu, cpuset);
	int ret = pthread_setaffinity_np(pthread_self(), cpuset_size(cpuset), cpuset);
	cpuset_destroy(cpuset);
	return ret;
}
#define SET_CPU_AFFINITY
#endif /* defined __NetBSD__ */

#ifndef GET_TOTAL_CPUS_DEFINED
static int get_total_cpus(void)
{
	static int warning_printed = 0;
	if (!warning_printed) {
		warning_printed = 1;
		warnf("Your system is not supported by libcpuid -- don't know how to detect the\n");
		warnf("total number of CPUs on your system. It will be reported as 1.\n");
		printf("Please use cpu_id_t.logical_cpus field instead.\n");
	}
	return 1;
}
#endif /* GET_TOTAL_CPUS_DEFINED */

#ifndef SET_CPU_AFFINITY
static int set_cpu_affinity(uint32_t logical_cpu)
{
	static int warning_printed = 0;
	if (!warning_printed) {
		warning_printed = 1;
		warnf("Your system is not supported by libcpuid -- don't know how to set the CPU affinity.\n");
	}
	return -1;
}
#endif /* SET_CPU_AFFINITY */

static int cpuid_serialize_raw_data_internal(struct cpu_raw_data_t* single_raw, struct cpu_raw_data_array_t *raw_array, const char* filename)
{
	int i;
	const bool raw_is_init = (raw_array != NULL) && raw_array->num_raw > 0;
	int8_t logical_cpu = raw_is_init ? 0 : -1;
	struct cpu_raw_data_t* raw_ptr = raw_is_init ? &raw_array->raw[0] : single_raw;
	FILE *f;

	/* Open file descriptor */
	f = !strcmp(filename, "") ? stdin : fopen(filename, "wt");
	if (!f)
		return set_error(ERR_OPEN);
	debugf(1, "Writing RAW dump to '%s'\n", f == stdin ? "stdin" : filename);

	/* Write RAW data to output file */
	fprintf(f, "version=%s\n", VERSION);
	while ((raw_is_init && (logical_cpu < raw_array->num_raw)) || (!raw_is_init && (logical_cpu < 0))) {
		if (raw_is_init) {
			debugf(2, "Writing RAW dump for logical CPU %i\n", logical_cpu);
			fprintf(f, "\n_________________ Logical CPU #%i _________________\n", logical_cpu);
			raw_ptr = &raw_array->raw[logical_cpu];
		}
		for (i = 0; i < MAX_CPUID_LEVEL; i++)
			fprintf(f, "basic_cpuid[%d]=%08x %08x %08x %08x\n", i,
				raw_ptr->basic_cpuid[i][EAX], raw_ptr->basic_cpuid[i][EBX],
				raw_ptr->basic_cpuid[i][ECX], raw_ptr->basic_cpuid[i][EDX]);
		for (i = 0; i < MAX_EXT_CPUID_LEVEL; i++)
			fprintf(f, "ext_cpuid[%d]=%08x %08x %08x %08x\n", i,
				raw_ptr->ext_cpuid[i][EAX], raw_ptr->ext_cpuid[i][EBX],
				raw_ptr->ext_cpuid[i][ECX], raw_ptr->ext_cpuid[i][EDX]);
		for (i = 0; i < MAX_INTELFN4_LEVEL; i++)
			fprintf(f, "intel_fn4[%d]=%08x %08x %08x %08x\n", i,
				raw_ptr->intel_fn4[i][EAX], raw_ptr->intel_fn4[i][EBX],
				raw_ptr->intel_fn4[i][ECX], raw_ptr->intel_fn4[i][EDX]);
		for (i = 0; i < MAX_INTELFN11_LEVEL; i++)
			fprintf(f, "intel_fn11[%d]=%08x %08x %08x %08x\n", i,
				raw_ptr->intel_fn11[i][EAX], raw_ptr->intel_fn11[i][EBX],
				raw_ptr->intel_fn11[i][ECX], raw_ptr->intel_fn11[i][EDX]);
		for (i = 0; i < MAX_INTELFN12H_LEVEL; i++)
			fprintf(f, "intel_fn12h[%d]=%08x %08x %08x %08x\n", i,
				raw_ptr->intel_fn12h[i][EAX], raw_ptr->intel_fn12h[i][EBX],
				raw_ptr->intel_fn12h[i][ECX], raw_ptr->intel_fn12h[i][EDX]);
		for (i = 0; i < MAX_INTELFN14H_LEVEL; i++)
			fprintf(f, "intel_fn14h[%d]=%08x %08x %08x %08x\n", i,
				raw_ptr->intel_fn14h[i][EAX], raw_ptr->intel_fn14h[i][EBX],
				raw_ptr->intel_fn14h[i][ECX], raw_ptr->intel_fn14h[i][EDX]);
		for (i = 0; i < MAX_AMDFN8000001DH_LEVEL; i++)
			fprintf(f, "amd_fn8000001dh[%d]=%08x %08x %08x %08x\n", i,
				raw_ptr->amd_fn8000001dh[i][EAX], raw_ptr->amd_fn8000001dh[i][EBX],
				raw_ptr->amd_fn8000001dh[i][ECX], raw_ptr->amd_fn8000001dh[i][EDX]);
		logical_cpu++;
	}

	/* Close file descriptor */
	if (strcmp(filename, ""))
		fclose(f);
	return set_error(ERR_OK);
}

#define RAW_ASSIGN_LINE(__line) __line[EAX] = eax ; __line[EBX] = ebx ; __line[ECX] = ecx ; __line[EDX] = edx
static int cpuid_deserialize_raw_data_internal(struct cpu_raw_data_t* single_raw, struct cpu_raw_data_array_t *raw_array, const char* filename)
{
	int i;
	int cur_line = 0;
	int assigned = 0;
	int subleaf = 0;
	bool is_libcpuid_dump = true;
	bool is_aida64_dump = false;
	const bool raw_is_init = (raw_array != NULL);
	int16_t logical_cpu = -1;
	uint32_t addr, eax, ebx, ecx, edx;
	char version[8];
	char line[100];
	struct cpu_raw_data_t* raw_ptr = raw_is_init ? &raw_array->raw[0] : single_raw;
	FILE *f;

	/* Open file descriptor */
	f = !strcmp(filename, "") ? stdin : fopen(filename, "rt");
	if (!f)
		return set_error(ERR_OPEN);
	debugf(1, "Opening RAW dump from '%s'\n", f == stdin ? "stdin" : filename);

	if (raw_is_init) {
		raw_array->with_affinity = false;
		raw_array->num_raw = raw_array->with_affinity ? 0 : 1;
		raw_data_t_constructor(raw_ptr);
	}

	/* Parse file and store data in cpu_raw_data_t */
	while (fgets(line, sizeof(line), f) != NULL) {
		line[strcspn(line, "\n")] = '\0';
		cur_line++;
		if (cur_line == 1) {
			if (sscanf(line, "version=%s", version) >= 1) {
				is_libcpuid_dump = true;
				is_aida64_dump = false;
				debugf(2, "Recognized version '%s' from RAW dump\n", version);
				continue;
			}
			else if (!strcmp(line, "------[ Versions ]------") || !strcmp(line, "------[ Logical CPU #0 ]------") || !strcmp(line, "------[ CPUID Registers / Logical CPU #0 ]------")) {
				is_libcpuid_dump = false;
				is_aida64_dump = true;
				debugf(2, "Recognized AIDA64 RAW dump\n");
			}
		}

		if (is_libcpuid_dump) {
			if (raw_is_init && (sscanf(line, "_________________ Logical CPU #%hi _________________", &logical_cpu) >= 1)) {
				if (raw_array->num_raw >= CPU_RAW_MAX) {
					warnf("RAW dump contains more than %d logical CPU, cannot parse more.\n", CPU_RAW_MAX);
					return set_error(ERR_NO_MEM);
				}
				debugf(2, "Parsing RAW dump for logical CPU %i\n", logical_cpu);
				raw_ptr = &raw_array->raw[logical_cpu];
				raw_array->with_affinity = true;
				raw_array->num_raw = logical_cpu + 1;
				raw_data_t_constructor(raw_ptr);
			}
			else if ((sscanf(line, "basic_cpuid[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_CPUID_LEVEL)) {
				RAW_ASSIGN_LINE(raw_ptr->basic_cpuid[i]);
			}
			else if ((sscanf(line, "ext_cpuid[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_EXT_CPUID_LEVEL)) {
				RAW_ASSIGN_LINE(raw_ptr->ext_cpuid[i]);
			}
			else if ((sscanf(line, "intel_fn4[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_INTELFN4_LEVEL)) {
				RAW_ASSIGN_LINE(raw_ptr->intel_fn4[i]);
			}
			else if ((sscanf(line, "intel_fn11[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_INTELFN11_LEVEL)) {
				RAW_ASSIGN_LINE(raw_ptr->intel_fn11[i]);
			}
			else if ((sscanf(line, "intel_fn12h[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_INTELFN12H_LEVEL)) {
				RAW_ASSIGN_LINE(raw_ptr->intel_fn12h[i]);
			}
			else if ((sscanf(line, "intel_fn14h[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_INTELFN14H_LEVEL)) {
				RAW_ASSIGN_LINE(raw_ptr->intel_fn14h[i]);
			}
			else if ((sscanf(line, "amd_fn8000001dh[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_AMDFN8000001DH_LEVEL)) {
				RAW_ASSIGN_LINE(raw_ptr->amd_fn8000001dh[i]);
			}
			else if (line[0] != '\0') {
				warnf("Warning: file '%s', line %d: '%s' not understood!\n", filename, cur_line, line);
			}
		}
		else if (is_aida64_dump) {
			if (raw_is_init && ((sscanf(line, "------[Logical CPU #%hi ]------", &logical_cpu) >= 1) || \
			                            (sscanf(line, "------[ CPUID Registers / Logical CPU #%hi ]------", &logical_cpu) >= 1))) {
				if (raw_array->num_raw >= CPU_RAW_MAX) {
					warnf("AIDA64 RAW dump contains more than %d logical CPU, cannot parse more.\n", CPU_RAW_MAX);
					return set_error(ERR_NO_MEM);
				}
				debugf(2, "Parsing AIDA64 RAW dump for logical CPU %i\n", logical_cpu);
				raw_ptr = &raw_array->raw[logical_cpu];
				raw_array->with_affinity = true;
				raw_array->num_raw = logical_cpu + 1;
				raw_data_t_constructor(raw_ptr);
				continue;
			}
			subleaf = 0;
			assigned = sscanf(line, "CPUID %x: %x-%x-%x-%x [SL %02i]", &addr, &eax, &ebx, &ecx, &edx, &subleaf);
			debugf(3, "RAW line %d: %i items assigned for string '%s'\n", cur_line, assigned, line);
			if ((assigned >= 5) && (subleaf == 0)) {
				if (addr < MAX_CPUID_LEVEL) {
					i = (int) addr;
					RAW_ASSIGN_LINE(raw_ptr->basic_cpuid[i]);
				}
				else if ((addr >= ADDRESS_EXT_CPUID_START) && (addr < ADDRESS_EXT_CPUID_END)) {
					i = (int) addr - ADDRESS_EXT_CPUID_START;
					RAW_ASSIGN_LINE(raw_ptr->ext_cpuid[i]);
				}
			}
			if (assigned >= 6) {
				i = subleaf;
				switch (addr) {
					case 0x00000004: RAW_ASSIGN_LINE(raw_ptr->intel_fn4[i]);       break;
					case 0x0000000B: RAW_ASSIGN_LINE(raw_ptr->intel_fn11[i]);      break;
					case 0x00000012: RAW_ASSIGN_LINE(raw_ptr->intel_fn12h[i]);     break;
					case 0x00000014: RAW_ASSIGN_LINE(raw_ptr->intel_fn14h[i]);     break;
					case 0x8000001D: RAW_ASSIGN_LINE(raw_ptr->amd_fn8000001dh[i]); break;
					default: break;
				}
			}
		}
	}

	/* Close file descriptor */
	if (strcmp(filename, ""))
		fclose(f);
	return set_error(ERR_OK);
}
#undef RAW_ASSIGN_LINE

static void load_features_common(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	const struct feature_map_t matchtable_edx1[] = {
		{  0, CPU_FEATURE_FPU },
		{  1, CPU_FEATURE_VME },
		{  2, CPU_FEATURE_DE },
		{  3, CPU_FEATURE_PSE },
		{  4, CPU_FEATURE_TSC },
		{  5, CPU_FEATURE_MSR },
		{  6, CPU_FEATURE_PAE },
		{  7, CPU_FEATURE_MCE },
		{  8, CPU_FEATURE_CX8 },
		{  9, CPU_FEATURE_APIC },
		{ 11, CPU_FEATURE_SEP },
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
		{  0, CPU_FEATURE_PNI },
		{  1, CPU_FEATURE_PCLMUL },
		{  3, CPU_FEATURE_MONITOR },
		{  9, CPU_FEATURE_SSSE3 },
		{ 12, CPU_FEATURE_FMA3 },
		{ 13, CPU_FEATURE_CX16 },
		{ 19, CPU_FEATURE_SSE4_1 },
		{ 20, CPU_FEATURE_SSE4_2 },
		{ 22, CPU_FEATURE_MOVBE },
		{ 23, CPU_FEATURE_POPCNT },
		{ 25, CPU_FEATURE_AES },
		{ 26, CPU_FEATURE_XSAVE },
		{ 27, CPU_FEATURE_OSXSAVE },
		{ 28, CPU_FEATURE_AVX },
		{ 29, CPU_FEATURE_F16C },
		{ 30, CPU_FEATURE_RDRAND },
	};
	const struct feature_map_t matchtable_ebx7[] = {
		{  3, CPU_FEATURE_BMI1 },
		{  5, CPU_FEATURE_AVX2 },
		{  8, CPU_FEATURE_BMI2 },
		{ 18, CPU_FEATURE_RDSEED },
		{ 19, CPU_FEATURE_ADX },
		{ 29, CPU_FEATURE_SHA_NI },
	};
	const struct feature_map_t matchtable_edx81[] = {
		{ 11, CPU_FEATURE_SYSCALL },
		{ 27, CPU_FEATURE_RDTSCP },
		{ 29, CPU_FEATURE_LM },
	};
	const struct feature_map_t matchtable_ecx81[] = {
		{  0, CPU_FEATURE_LAHF_LM },
		{  5, CPU_FEATURE_ABM },
	};
	const struct feature_map_t matchtable_edx87[] = {
		{  8, CPU_FEATURE_CONSTANT_TSC },
	};
	if (raw->basic_cpuid[0][EAX] >= 1) {
		match_features(matchtable_edx1, COUNT_OF(matchtable_edx1), raw->basic_cpuid[1][EDX], data);
		match_features(matchtable_ecx1, COUNT_OF(matchtable_ecx1), raw->basic_cpuid[1][ECX], data);
	}
	if (raw->basic_cpuid[0][EAX] >= 7) {
		match_features(matchtable_ebx7, COUNT_OF(matchtable_ebx7), raw->basic_cpuid[7][EBX], data);
	}
	if (raw->ext_cpuid[0][EAX] >= 0x80000001) {
		match_features(matchtable_edx81, COUNT_OF(matchtable_edx81), raw->ext_cpuid[1][EDX], data);
		match_features(matchtable_ecx81, COUNT_OF(matchtable_ecx81), raw->ext_cpuid[1][ECX], data);
	}
	if (raw->ext_cpuid[0][EAX] >= 0x80000007) {
		match_features(matchtable_edx87, COUNT_OF(matchtable_edx87), raw->ext_cpuid[7][EDX], data);
	}
	if (data->flags[CPU_FEATURE_SSE]) {
		/* apply guesswork to check if the SSE unit width is 128 bit */
		switch (data->vendor) {
			case VENDOR_AMD:
				data->sse_size = (data->ext_family >= 16 && data->ext_family != 17) ? 128 : 64;
				break;
			case VENDOR_INTEL:
				data->sse_size = (data->family == 6 && data->ext_model >= 15) ? 128 : 64;
				break;
			default:
				break;
		}
		/* leave the CPU_FEATURE_128BIT_SSE_AUTH 0; the advanced per-vendor detection routines
		 * will set it accordingly if they detect the needed bit */
	}
}

static cpu_vendor_t cpuid_vendor_identify(const uint32_t *raw_vendor, char *vendor_str)
{
	int i;
	cpu_vendor_t vendor = VENDOR_UNKNOWN;
	const struct { cpu_vendor_t vendor; char match[16]; }
	matchtable[NUM_CPU_VENDORS] = {
		/* source: http://www.sandpile.org/ia32/cpuid.htm */
		{ VENDOR_INTEL		, "GenuineIntel" },
		{ VENDOR_AMD		, "AuthenticAMD" },
		{ VENDOR_CYRIX		, "CyrixInstead" },
		{ VENDOR_NEXGEN		, "NexGenDriven" },
		{ VENDOR_TRANSMETA	, "GenuineTMx86" },
		{ VENDOR_UMC		, "UMC UMC UMC " },
		{ VENDOR_CENTAUR	, "CentaurHauls" },
		{ VENDOR_RISE		, "RiseRiseRise" },
		{ VENDOR_SIS		, "SiS SiS SiS " },
		{ VENDOR_NSC		, "Geode by NSC" },
		{ VENDOR_HYGON		, "HygonGenuine" },
	};

	memcpy(vendor_str + 0, &raw_vendor[1], 4);
	memcpy(vendor_str + 4, &raw_vendor[3], 4);
	memcpy(vendor_str + 8, &raw_vendor[2], 4);
	vendor_str[12] = 0;

	/* Determine vendor: */
	for (i = 0; i < NUM_CPU_VENDORS; i++)
		if (!strcmp(vendor_str, matchtable[i].match)) {
			vendor = matchtable[i].vendor;
			break;
		}
	return vendor;
}

static int cpuid_basic_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int i, j, basic, xmodel, xfamily, ext;
	char brandstr[64] = {0};
	data->vendor = cpuid_vendor_identify(raw->basic_cpuid[0], data->vendor_str);

	if (data->vendor == VENDOR_UNKNOWN)
		return set_error(ERR_CPU_UNKN);
	data->architecture = ARCHITECTURE_X86;
	basic = raw->basic_cpuid[0][EAX];
	if (basic >= 1) {
		data->family = (raw->basic_cpuid[1][EAX] >> 8) & 0xf;
		data->model = (raw->basic_cpuid[1][EAX] >> 4) & 0xf;
		data->stepping = raw->basic_cpuid[1][EAX] & 0xf;
		xmodel = (raw->basic_cpuid[1][EAX] >> 16) & 0xf;
		xfamily = (raw->basic_cpuid[1][EAX] >> 20) & 0xff;
		if (data->vendor == VENDOR_AMD && data->family < 0xf)
			data->ext_family = data->family;
		else
			data->ext_family = data->family + xfamily;
		data->ext_model = data->model + (xmodel << 4);
	}
	ext = raw->ext_cpuid[0][EAX] - 0x8000000;

	/* obtain the brand string, if present: */
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
	data->total_logical_cpus = get_total_cpus();
	return set_error(ERR_OK);
}

static void make_list_from_string(const char* csv, struct cpu_list_t* list)
{
	int i, j, n, l, last;
	l = (int) strlen(csv);
	n = 0;
	for (i = 0; i < l; i++) if (csv[i] == ',') n++;
	n++;
	list->names = (char**) malloc(sizeof(char*) * n);
	if (!list->names) { /* Memory allocation failed */
		list->num_entries = 0;
		set_error(ERR_NO_MEM);
		return;
	}
	list->num_entries = n;
	last = -1;
	n = 0;
	for (i = 0; i <= l; i++) if (i == l || csv[i] == ',') {
		list->names[n] = (char*) malloc(i - last);
		if (!list->names[n]) { /* Memory allocation failed */
			set_error(ERR_NO_MEM);
			for (j = 0; j < n; j++) free(list->names[j]);
			free(list->names);
			list->num_entries = 0;
			list->names = NULL;
			return;
		}
		memcpy(list->names[n], &csv[last + 1], i - last - 1);
		list->names[n][i - last - 1] = '\0';
		n++;
		last = i;
	}
}


/* Interface: */

int cpuid_get_total_cpus(void)
{
	return get_total_cpus();
}

int cpuid_present(void)
{
	return cpuid_exists_by_eflags();
}

void cpu_exec_cpuid(uint32_t eax, uint32_t* regs)
{
	regs[0] = eax;
	regs[1] = regs[2] = regs[3] = 0;
	exec_cpuid(regs);
}

void cpu_exec_cpuid_ext(uint32_t* regs)
{
	exec_cpuid(regs);
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
	for (i = 0; i < MAX_INTELFN4_LEVEL; i++) {
		memset(data->intel_fn4[i], 0, sizeof(data->intel_fn4[i]));
		data->intel_fn4[i][EAX] = 4;
		data->intel_fn4[i][ECX] = i;
		cpu_exec_cpuid_ext(data->intel_fn4[i]);
	}
	for (i = 0; i < MAX_INTELFN11_LEVEL; i++) {
		memset(data->intel_fn11[i], 0, sizeof(data->intel_fn11[i]));
		data->intel_fn11[i][EAX] = 11;
		data->intel_fn11[i][ECX] = i;
		cpu_exec_cpuid_ext(data->intel_fn11[i]);
	}
	for (i = 0; i < MAX_INTELFN12H_LEVEL; i++) {
		memset(data->intel_fn12h[i], 0, sizeof(data->intel_fn12h[i]));
		data->intel_fn12h[i][EAX] = 0x12;
		data->intel_fn12h[i][ECX] = i;
		cpu_exec_cpuid_ext(data->intel_fn12h[i]);
	}
	for (i = 0; i < MAX_INTELFN14H_LEVEL; i++) {
		memset(data->intel_fn14h[i], 0, sizeof(data->intel_fn14h[i]));
		data->intel_fn14h[i][EAX] = 0x14;
		data->intel_fn14h[i][ECX] = i;
		cpu_exec_cpuid_ext(data->intel_fn14h[i]);
	}
	for (i = 0; i < MAX_AMDFN8000001DH_LEVEL; i++) {
		memset(data->amd_fn8000001dh[i], 0, sizeof(data->amd_fn8000001dh[i]));
		data->amd_fn8000001dh[i][EAX] = 0x8000001d;
		data->amd_fn8000001dh[i][ECX] = i;
		cpu_exec_cpuid_ext(data->amd_fn8000001dh[i]);
	}
	return set_error(ERR_OK);
}

int cpuid_get_all_raw_data(struct cpu_raw_data_array_t *data)
{
	int cur_error = set_error(ERR_OK);
	int ret_error = set_error(ERR_OK);
	struct cpu_raw_data_t* raw_ptr = NULL;

	if (data == NULL)
		return set_error(ERR_HANDLE);

	data->with_affinity = true;
	data->num_raw = 0;
	while (set_cpu_affinity(data->num_raw) == 0) {
		if (data->num_raw >= CPU_RAW_MAX) {
			warnf("System has more than %d logical CPU, cannot get more RAW.\n", CPU_RAW_MAX);
			return set_error(ERR_NO_MEM);
		}
		debugf(2, "Getting RAW dump for logical CPU %i\n", data->num_raw);
		raw_ptr = &data->raw[data->num_raw];
		raw_data_t_constructor(raw_ptr);
		cur_error = cpuid_get_raw_data(raw_ptr);
		if (ret_error == ERR_OK)
			ret_error = cur_error;
		data->num_raw++;
	}

	return ret_error;
}

int cpuid_serialize_raw_data(struct cpu_raw_data_t* data, const char* filename)
{
	return cpuid_serialize_raw_data_internal(data, NULL, filename);
}

int cpuid_serialize_all_raw_data(struct cpu_raw_data_array_t *data, const char* filename)
{
	return cpuid_serialize_raw_data_internal(NULL, data, filename);
}

int cpuid_deserialize_raw_data(struct cpu_raw_data_t* data, const char* filename)
{
	raw_data_t_constructor(data);
	return cpuid_deserialize_raw_data_internal(data, NULL, filename);
}

int cpuid_deserialize_all_raw_data(struct cpu_raw_data_array_t *data, const char* filename)
{
	return cpuid_deserialize_raw_data_internal(NULL, data, filename);
}

int cpu_ident_internal(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct internal_id_info_t* internal)
{
	int r;
	struct cpu_raw_data_t myraw;
	if (!raw) {
		if ((r = cpuid_get_raw_data(&myraw)) < 0)
			return set_error(r);
		raw = &myraw;
	}
	cpu_id_t_constructor(data);
	internal->smt_id = -1;
	internal->core_id = -1;
	if ((r = cpuid_basic_identify(raw, data)) < 0)
		return set_error(r);
	switch (data->vendor) {
		case VENDOR_INTEL:
			r = cpuid_identify_intel(raw, data, internal);
			break;
		case VENDOR_AMD:
		case VENDOR_HYGON:
			r = cpuid_identify_amd(raw, data, internal);
			break;
		default:
			break;
	}
	/* Backward compatibility */
	/* - Deprecated since v0.5.0 */
	data->l1_assoc     = data->l1_data_assoc;
	data->l1_cacheline = data->l1_data_cacheline;
	return set_error(r);
}

int cpu_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int r;
	struct internal_id_info_t throwaway;
	r = cpu_ident_internal(raw, data, &throwaway);
	data->affinity_mask = (1ULL << data->num_logical_cpus) - 1;
	return r;
}

int cpu_identify_all(struct cpu_raw_data_array_t *raw_array, struct system_id_t* system)
{
	int cur_error = set_error(ERR_OK);
	int ret_error = set_error(ERR_OK);	bool is_new_cpu_type;
	uint8_t logical_cpu;
	uint8_t cpu_type_index = 0;
	int32_t core_previous_id = -1;
	int32_t num_cores = 1;
	int32_t num_logical_cpus = 1;
	uint32_t affinity_mask = 0x00000001;
	struct cpu_raw_data_array_t my_raw_array;
	struct internal_id_info_t throwaway;

	if (!raw_array) {
		if ((ret_error = cpuid_get_all_raw_data(&my_raw_array)) < 0)
			return set_error(ret_error);
		raw_array = &my_raw_array;
	}
	if (system == NULL)
		return set_error(ERR_HANDLE);
	system->num_cpu_types = 0;

	/* Iterate over all RAW */
	for (logical_cpu = 0; logical_cpu < raw_array->num_raw; logical_cpu++) {
		debugf(2, "Identifying logical core %u...\n", logical_cpu);
		is_new_cpu_type = false;
		cur_error = cpu_ident_internal(&raw_array->raw[logical_cpu], &system->cpu_types[system->num_cpu_types], &throwaway);
		if (ret_error == ERR_OK)
			ret_error = cur_error;

		/* Copy data to system->cpu_types on the first iteration or when purpose is different than previous core */
		if ((system->num_cpu_types == 0) || (system->cpu_types[system->num_cpu_types].purpose != system->cpu_types[system->num_cpu_types - 1].purpose)) {
			is_new_cpu_type = true;
			system->num_cpu_types++;
		}
		/* Increment logical and physical CPU counters for current purpose */
		else {
			affinity_mask |= 1ULL << logical_cpu;
			num_logical_cpus++;
			if (core_previous_id != throwaway.core_id)
				num_cores++;
		}
		/* Update logical and physical CPU counters in system->cpu_types on the last iteration or when purpose is different than previous core */
		if ((logical_cpu + 1 == raw_array->num_raw) || (is_new_cpu_type && (system->num_cpu_types > 1))) {
			cpu_type_index = is_new_cpu_type ? system->num_cpu_types - 2 : system->num_cpu_types - 1;
			/* Save current values in system->cpu_types[cpu_type_index] */
			system->cpu_types[cpu_type_index].affinity_mask = affinity_mask;
			if (core_previous_id > 0) {
				system->cpu_types[cpu_type_index].num_cores = num_cores;
				system->cpu_types[cpu_type_index].num_logical_cpus = num_logical_cpus;
			}
			/* Reset values for the next purpose */
			affinity_mask = 1ULL << logical_cpu;
			num_cores = 1;
			num_logical_cpus = 1;
		}
		core_previous_id = throwaway.core_id;
	}

	/* Update the total_logical_cpus value for each purpose */
	for (cpu_type_index = 0; cpu_type_index < system->num_cpu_types; cpu_type_index++)
		system->cpu_types[cpu_type_index].total_logical_cpus = logical_cpu;

	return ret_error;
}

struct cpu_id_t* cpu_request_core_type(cpu_purpose_t purpose, struct cpu_raw_data_array_t* raw_array, struct system_id_t* system)
{
	uint8_t cpu_type_index = 0;
	struct system_id_t my_system;

	if (!system) {
		system = &my_system;
		if (cpu_identify_all(raw_array, system) != ERR_OK)
			return NULL;
	}

	for (cpu_type_index = 0; cpu_type_index < system->num_cpu_types; cpu_type_index++) {
		if (purpose == system->cpu_types[cpu_type_index].purpose)
			return &system->cpu_types[cpu_type_index];
	}

	set_error(ERR_NOT_FOUND);
	return NULL;
}

const char* cpu_architecture_str(cpu_architecture_t architecture)
{
	const struct { cpu_architecture_t architecture; const char* name; }
	matchtable[] = {
		{ ARCHITECTURE_UNKNOWN, "unknown" },
		{ ARCHITECTURE_X86,     "x86"     },
		{ ARCHITECTURE_ARM,     "ARM"     },
	};
	unsigned i, n = COUNT_OF(matchtable);
	if (n != NUM_CPU_ARCHITECTURES + 1) {
		warnf("Warning: incomplete library, architecture matchtable size differs from the actual number of architectures.\n");
	}
	for (i = 0; i < n; i++)
		if (matchtable[i].architecture == architecture)
			return matchtable[i].name;
	return "";
}

const char* cpu_purpose_str(cpu_purpose_t purpose)
{
	const struct { cpu_purpose_t purpose; const char* name; }
	matchtable[] = {
		{ PURPOSE_GENERAL,     "general"     },
		{ PURPOSE_PERFORMANCE, "performance" },
		{ PURPOSE_EFFICIENCY,  "efficiency"  },
	};
	unsigned i, n = COUNT_OF(matchtable);
	if (n != NUM_CPU_PURPOSES) {
		warnf("Warning: incomplete library, purpose matchtable size differs from the actual number of purposes.\n");
	}
	for (i = 0; i < n; i++)
		if (matchtable[i].purpose == purpose)
			return matchtable[i].name;
	return "";
}

const char* cpu_feature_str(cpu_feature_t feature)
{
	const struct { cpu_feature_t feature; const char* name; }
	matchtable[] = {
		{ CPU_FEATURE_FPU, "fpu" },
		{ CPU_FEATURE_VME, "vme" },
		{ CPU_FEATURE_DE, "de" },
		{ CPU_FEATURE_PSE, "pse" },
		{ CPU_FEATURE_TSC, "tsc" },
		{ CPU_FEATURE_MSR, "msr" },
		{ CPU_FEATURE_PAE, "pae" },
		{ CPU_FEATURE_MCE, "mce" },
		{ CPU_FEATURE_CX8, "cx8" },
		{ CPU_FEATURE_APIC, "apic" },
		{ CPU_FEATURE_MTRR, "mtrr" },
		{ CPU_FEATURE_SEP, "sep" },
		{ CPU_FEATURE_PGE, "pge" },
		{ CPU_FEATURE_MCA, "mca" },
		{ CPU_FEATURE_CMOV, "cmov" },
		{ CPU_FEATURE_PAT, "pat" },
		{ CPU_FEATURE_PSE36, "pse36" },
		{ CPU_FEATURE_PN, "pn" },
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
		{ CPU_FEATURE_PNI, "pni" },
		{ CPU_FEATURE_PCLMUL, "pclmul" },
		{ CPU_FEATURE_DTS64, "dts64" },
		{ CPU_FEATURE_MONITOR, "monitor" },
		{ CPU_FEATURE_DS_CPL, "ds_cpl" },
		{ CPU_FEATURE_VMX, "vmx" },
		{ CPU_FEATURE_SMX, "smx" },
		{ CPU_FEATURE_EST, "est" },
		{ CPU_FEATURE_TM2, "tm2" },
		{ CPU_FEATURE_SSSE3, "ssse3" },
		{ CPU_FEATURE_CID, "cid" },
		{ CPU_FEATURE_CX16, "cx16" },
		{ CPU_FEATURE_XTPR, "xtpr" },
		{ CPU_FEATURE_PDCM, "pdcm" },
		{ CPU_FEATURE_DCA, "dca" },
		{ CPU_FEATURE_SSE4_1, "sse4_1" },
		{ CPU_FEATURE_SSE4_2, "sse4_2" },
		{ CPU_FEATURE_SYSCALL, "syscall" },
		{ CPU_FEATURE_XD, "xd" },
		{ CPU_FEATURE_X2APIC, "x2apic"},
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
		{ CPU_FEATURE_FXSR_OPT, "fxsr_opt" },
		{ CPU_FEATURE_RDTSCP, "rdtscp" },
		{ CPU_FEATURE_LM, "lm" },
		{ CPU_FEATURE_LAHF_LM, "lahf_lm" },
		{ CPU_FEATURE_CMP_LEGACY, "cmp_legacy" },
		{ CPU_FEATURE_SVM, "svm" },
		{ CPU_FEATURE_SSE4A, "sse4a" },
		{ CPU_FEATURE_MISALIGNSSE, "misalignsse" },
		{ CPU_FEATURE_ABM, "abm" },
		{ CPU_FEATURE_3DNOWPREFETCH, "3dnowprefetch" },
		{ CPU_FEATURE_OSVW, "osvw" },
		{ CPU_FEATURE_IBS, "ibs" },
		{ CPU_FEATURE_SSE5, "sse5" },
		{ CPU_FEATURE_SKINIT, "skinit" },
		{ CPU_FEATURE_WDT, "wdt" },
		{ CPU_FEATURE_TS, "ts" },
		{ CPU_FEATURE_FID, "fid" },
		{ CPU_FEATURE_VID, "vid" },
		{ CPU_FEATURE_TTP, "ttp" },
		{ CPU_FEATURE_TM_AMD, "tm_amd" },
		{ CPU_FEATURE_STC, "stc" },
		{ CPU_FEATURE_100MHZSTEPS, "100mhzsteps" },
		{ CPU_FEATURE_HWPSTATE, "hwpstate" },
		{ CPU_FEATURE_CONSTANT_TSC, "constant_tsc" },
		{ CPU_FEATURE_XOP, "xop" },
		{ CPU_FEATURE_FMA3, "fma3" },
		{ CPU_FEATURE_FMA4, "fma4" },
		{ CPU_FEATURE_TBM, "tbm" },
		{ CPU_FEATURE_F16C, "f16c" },
		{ CPU_FEATURE_RDRAND, "rdrand" },
		{ CPU_FEATURE_CPB, "cpb" },
		{ CPU_FEATURE_APERFMPERF, "aperfmperf" },
		{ CPU_FEATURE_PFI, "pfi" },
		{ CPU_FEATURE_PA, "pa" },
		{ CPU_FEATURE_AVX2, "avx2" },
		{ CPU_FEATURE_BMI1, "bmi1" },
		{ CPU_FEATURE_BMI2, "bmi2" },
		{ CPU_FEATURE_HLE, "hle" },
		{ CPU_FEATURE_RTM, "rtm" },
		{ CPU_FEATURE_AVX512F, "avx512f" },
		{ CPU_FEATURE_AVX512DQ, "avx512dq" },
		{ CPU_FEATURE_AVX512PF, "avx512pf" },
		{ CPU_FEATURE_AVX512ER, "avx512er" },
		{ CPU_FEATURE_AVX512CD, "avx512cd" },
		{ CPU_FEATURE_SHA_NI, "sha_ni" },
		{ CPU_FEATURE_AVX512BW, "avx512bw" },
		{ CPU_FEATURE_AVX512VL, "avx512vl" },
		{ CPU_FEATURE_SGX, "sgx" },
		{ CPU_FEATURE_RDSEED, "rdseed" },
		{ CPU_FEATURE_ADX, "adx" },
		{ CPU_FEATURE_AVX512VNNI, "avx512vnni" },
		{ CPU_FEATURE_AVX512VBMI, "avx512vbmi" },
		{ CPU_FEATURE_AVX512VBMI2, "avx512vbmi2" },
	};
	unsigned i, n = COUNT_OF(matchtable);
	if (n != NUM_CPU_FEATURES) {
		warnf("Warning: incomplete library, feature matchtable size differs from the actual number of features.\n");
	}
	for (i = 0; i < n; i++)
		if (matchtable[i].feature == feature)
			return matchtable[i].name;
	return "";
}

const char* cpuid_error(void)
{
	const struct { cpu_error_t error; const char *description; }
	matchtable[] = {
		{ ERR_OK       , "No error"},
		{ ERR_NO_CPUID , "CPUID instruction is not supported"},
		{ ERR_NO_RDTSC , "RDTSC instruction is not supported"},
		{ ERR_NO_MEM   , "Memory allocation failed"},
		{ ERR_OPEN     , "File open operation failed"},
		{ ERR_BADFMT   , "Bad file format"},
		{ ERR_NOT_IMP  , "Not implemented"},
		{ ERR_CPU_UNKN , "Unsupported processor"},
		{ ERR_NO_RDMSR , "RDMSR instruction is not supported"},
		{ ERR_NO_DRIVER, "RDMSR driver error (generic)"},
		{ ERR_NO_PERMS , "No permissions to install RDMSR driver"},
		{ ERR_EXTRACT  , "Cannot extract RDMSR driver (read only media?)"},
		{ ERR_HANDLE   , "Bad handle"},
		{ ERR_INVMSR   , "Invalid MSR"},
		{ ERR_INVCNB   , "Invalid core number"},
		{ ERR_HANDLE_R , "Error on handle read"},
		{ ERR_INVRANGE , "Invalid given range"},
		{ ERR_NOT_FOUND, "Requested type not found"},
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

libcpuid_warn_fn_t cpuid_set_warn_function(libcpuid_warn_fn_t new_fn)
{
	libcpuid_warn_fn_t ret = _warn_fun;
	_warn_fun = new_fn;
	return ret;
}

void cpuid_set_verbosiness_level(int level)
{
	_current_verboselevel = level;
}

cpu_vendor_t cpuid_get_vendor(void)
{
	static cpu_vendor_t vendor = VENDOR_UNKNOWN;
	uint32_t raw_vendor[4];
	char vendor_str[VENDOR_STR_MAX];

	if(vendor == VENDOR_UNKNOWN) {
		if (!cpuid_present())
			set_error(ERR_NO_CPUID);
		else {
			cpu_exec_cpuid(0, raw_vendor);
			vendor = cpuid_vendor_identify(raw_vendor, vendor_str);
		}
	}
	return vendor;
}

void cpuid_get_cpu_list(cpu_vendor_t vendor, struct cpu_list_t* list)
{
	switch (vendor) {
		case VENDOR_INTEL:
			cpuid_get_list_intel(list);
			break;
		case VENDOR_AMD:
		case VENDOR_HYGON:
			cpuid_get_list_amd(list);
			break;
		case VENDOR_CYRIX:
			make_list_from_string("Cx486,Cx5x86,6x86,6x86MX,M II,MediaGX,MediaGXi,MediaGXm", list);
			break;
		case VENDOR_NEXGEN:
			make_list_from_string("Nx586", list);
			break;
		case VENDOR_TRANSMETA:
			make_list_from_string("Crusoe,Efficeon", list);
			break;
		case VENDOR_UMC:
			make_list_from_string("UMC x86 CPU", list);
			break;
		case VENDOR_CENTAUR:
			make_list_from_string("VIA C3,VIA C7,VIA Nano", list);
			break;
		case VENDOR_RISE:
			make_list_from_string("Rise mP6", list);
			break;
		case VENDOR_SIS:
			make_list_from_string("SiS mP6", list);
			break;
		case VENDOR_NSC:
			make_list_from_string("Geode GXm,Geode GXLV,Geode GX1,Geode GX2", list);
			break;
		default:
			warnf("Unknown vendor passed to cpuid_get_cpu_list()\n");
			set_error(ERR_INVRANGE);
			list->num_entries = 0;
			list->names = NULL;
			break;
 	}
}

void cpuid_free_cpu_list(struct cpu_list_t* list)
{
	int i;
	if (list->num_entries <= 0) return;
	for (i = 0; i < list->num_entries; i++)
		free(list->names[i]);
	free(list->names);
}
