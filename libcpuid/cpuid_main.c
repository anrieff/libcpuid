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
#include "recog_amd.h"
#include "recog_arm.h"
#include "recog_centaur.h"
#include "recog_intel.h"
#include "asm-bits.h"
#include "libcpuid_util.h"
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#ifdef HAVE_GETAUXVAL
# include <sys/auxv.h>
#endif /* HAVE_GETAUXVAL */
#ifdef HAVE_ELF_AUX_INFO
# include <sys/auxv.h>
#endif /* HAVE_ELF_AUX_INFO */

/* Implementation: */

#if defined(__STDC_VERSION__) && __STDC_VERSION__ == 201112L
#define INTERNAL_SCOPE _Thread_local
#elif defined(__GNUC__) // Also works for clang
#define INTERNAL_SCOPE __thread
#else
#define INTERNAL_SCOPE static
#endif

INTERNAL_SCOPE int _libcpuid_errno = ERR_OK;

int cpuid_set_error(cpu_error_t err)
{
	_libcpuid_errno = (int) err;
	return (int) err;
}

int cpuid_get_error()
{
	return _libcpuid_errno;
}

static void raw_data_t_constructor(struct cpu_raw_data_t* raw)
{
	memset(raw, 0, sizeof(struct cpu_raw_data_t));
}

static void cpu_id_t_constructor(struct cpu_id_t* id)
{
	memset(id, 0, sizeof(struct cpu_id_t));
	id->architecture = ARCHITECTURE_UNKNOWN;
	id->vendor = VENDOR_UNKNOWN;
	id->l1_data_cache = id->l1_instruction_cache = id->l2_cache = id->l3_cache = id->l4_cache = -1;
	id->l1_assoc = id->l1_data_assoc = id->l1_instruction_assoc = id->l2_assoc = id->l3_assoc = id->l4_assoc = -1;
	id->l1_cacheline = id->l1_data_cacheline = id->l1_instruction_cacheline = id->l2_cacheline = id->l3_cacheline = id->l4_cacheline = -1;
	id->l1_data_instances = id->l1_instruction_instances = id->l2_instances = id->l3_instances = id->l4_instances = -1;
	id->sse_size = -1;
	init_affinity_mask(&id->affinity_mask);
	id->purpose = PURPOSE_GENERAL;
}

static void cpu_raw_data_array_t_constructor(struct cpu_raw_data_array_t* raw_array, bool with_affinity)
{
	raw_array->with_affinity = with_affinity;
	raw_array->num_raw = 0;
	raw_array->raw = NULL;
}

static void system_id_t_constructor(struct system_id_t* system)
{
	system->num_cpu_types                  = 0;
	system->cpu_types                      = NULL;
	system->l1_data_total_instances        = -1;
	system->l1_instruction_total_instances = -1;
	system->l2_total_instances             = -1;
	system->l3_total_instances             = -1;
	system->l4_total_instances             = -1;
}

static void topology_t_constructor(struct internal_topology_t* topology, logical_cpu_t logical_cpu)
{
	memset(topology, 0, sizeof(struct internal_topology_t));
	topology->apic_id     = -1;
	topology->package_id  = -1;
	topology->core_id     = -1;
	topology->smt_id      = -1;
	topology->logical_cpu = logical_cpu;
}

static void core_instances_t_constructor(struct internal_core_instances_t* data)
{
	data->instances = 0;
	memset(data->htable, 0, sizeof(data->htable));
}

static void cache_instances_t_constructor(struct internal_cache_instances_t* data)
{
	memset(data->instances, 0, sizeof(data->instances));
	memset(data->htable,    0, sizeof(data->htable));
}

static void type_info_array_t_constructor(struct internal_type_info_array_t* data)
{
	data->num  = 0;
	data->data = NULL;
}

static int16_t cpuid_find_index_system_id(struct system_id_t* system, cpu_purpose_t purpose,
                                          struct internal_type_info_array_t* type_info, int32_t package_id, bool is_topology_supported)
{
	int16_t i = 0;

	if (is_topology_supported) {
		for (i = 0; i < system->num_cpu_types; i++)
			if ((system->cpu_types[i].purpose == purpose) && (type_info->data[i].package_id == package_id))
				return i;
	}
	else {
		for (i = 0; i < system->num_cpu_types; i++)
			if (system->cpu_types[i].purpose == purpose)
				return i;
	}

	return -1;
}

static void cpuid_grow_raw_data_array(struct cpu_raw_data_array_t* raw_array, logical_cpu_t n)
{
	logical_cpu_t i;
	struct cpu_raw_data_t *tmp = NULL;

	if ((n <= 0) || (n < raw_array->num_raw)) return;
	debugf(3, "Growing cpu_raw_data_array_t from %u to %u items\n", raw_array->num_raw, n);
	tmp = realloc(raw_array->raw, sizeof(struct cpu_raw_data_t) * n);
	if (tmp == NULL) { /* Memory allocation failure */
		cpuid_set_error(ERR_NO_MEM);
		return;
	}

	for (i = raw_array->num_raw; i < n; i++)
		raw_data_t_constructor(&tmp[i]);
	raw_array->num_raw = n;
	raw_array->raw     = tmp;
}

static void cpuid_grow_system_id(struct system_id_t* system, uint8_t n)
{
	uint8_t i;
	struct cpu_id_t *tmp = NULL;

	if ((n <= 0) || (n < system->num_cpu_types)) return;
	debugf(3, "Growing system_id_t from %u to %u items\n", system->num_cpu_types, n);
	tmp = realloc(system->cpu_types, sizeof(struct cpu_id_t) * n);
	if (tmp == NULL) { /* Memory allocation failure */
		cpuid_set_error(ERR_NO_MEM);
		return;
	}

	for (i = system->num_cpu_types; i < n; i++)
		cpu_id_t_constructor(&tmp[i]);
	system->num_cpu_types = n;
	system->cpu_types     = tmp;
}

static void cpuid_grow_type_info(struct internal_type_info_array_t* type_info, uint8_t n)
{
	uint8_t i;
	struct internal_type_info_t *tmp = NULL;

	if ((n <= 0) || (n < type_info->num)) return;
	debugf(3, "Growing internal_type_info_t from %u to %u items\n", type_info->num, n);
	tmp = realloc(type_info->data, sizeof(struct internal_type_info_t) * n);
	if (tmp == NULL) { /* Memory allocation failure */
		cpuid_set_error(ERR_NO_MEM);
		return;
	}

	for (i = type_info->num; i < n; i++) {
		core_instances_t_constructor(&tmp[i].core_instances);
		cache_instances_t_constructor(&tmp[i].cache_instances);
	}
	type_info->num  = n;
	type_info->data = tmp;
}

static void cpuid_free_type_info(struct internal_type_info_array_t* type_info)
{
	if (type_info->num <= 0) return;
	free(type_info->data);
	type_info->num = 0;
}

static cpu_architecture_t cpuid_architecture_identify(struct cpu_raw_data_t* raw)
{
	if (raw->basic_cpuid[0][EAX] != 0x0 || raw->basic_cpuid[0][EBX] != 0x0 || raw->basic_cpuid[0][ECX] != 0x0 || raw->basic_cpuid[0][EDX] != 0x0)
		return ARCHITECTURE_X86;
	else if (raw->arm_midr != 0x0)
		return ARCHITECTURE_ARM;

	return ARCHITECTURE_UNKNOWN;
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

INTERNAL_SCOPE thread_affinity_policy_data_t saved_affinity;

static bool save_cpu_affinity(void)
{
	mach_msg_type_number_t count = THREAD_AFFINITY_POLICY_COUNT;
	boolean_t get_default = false;
	return thread_policy_get(mach_thread_self(), THREAD_AFFINITY_POLICY, (thread_policy_t) &saved_affinity, &count, &get_default) == KERN_SUCCESS;
}

static bool restore_cpu_affinity(void)
{
	return thread_policy_set(mach_thread_self(), THREAD_AFFINITY_POLICY, (thread_policy_t) &saved_affinity, THREAD_AFFINITY_POLICY_COUNT) == KERN_SUCCESS;
}
#define PRESERVE_CPU_AFFINITY

static bool set_cpu_affinity(logical_cpu_t logical_cpu)
{
	thread_affinity_policy_data_t ap;
	ap.affinity_tag = logical_cpu + 1;
	/* Note: thread_policy_set() always returns KERN_SUCCESS even if the target logical CPU does not exist
	   Refer to #178 */
	if (logical_cpu >= get_total_cpus())
		return false;
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

#if (_WIN32_WINNT >= 0x0601)
INTERNAL_SCOPE GROUP_AFFINITY savedGroupAffinity;
#else
INTERNAL_SCOPE DWORD_PTR savedAffinityMask = 0;
#endif

static bool save_cpu_affinity(void)
{
#if (_WIN32_WINNT >= 0x0601)
	HANDLE thread = GetCurrentThread();
	return GetThreadGroupAffinity(thread, &savedGroupAffinity);
#else
/* Credits to https://stackoverflow.com/questions/6601862/query-thread-not-process-processor-affinity#6601917 */
	HANDLE thread = GetCurrentThread();
	DWORD_PTR threadAffinityMask = 1;
	while (threadAffinityMask) {
		savedAffinityMask = SetThreadAffinityMask(thread, threadAffinityMask);
		if(savedAffinityMask)
			return SetThreadAffinityMask(thread, savedAffinityMask);
		else if (GetLastError() != ERROR_INVALID_PARAMETER)
			return false;

		threadAffinityMask <<= 1; // try next CPU
	}
	return false;
#endif
}

static bool restore_cpu_affinity(void)
{
#if (_WIN32_WINNT >= 0x0601)
	if (!savedGroupAffinity.Mask)
		return false;

	HANDLE thread = GetCurrentThread();
	return SetThreadGroupAffinity(thread, &savedGroupAffinity, NULL);
#else
	if (!savedAffinityMask)
		return false;

	HANDLE thread = GetCurrentThread();
	return SetThreadAffinityMask(thread, savedAffinityMask);
#endif
}
#define PRESERVE_CPU_AFFINITY

static bool set_cpu_affinity(logical_cpu_t logical_cpu)
{
/* Credits to https://github.com/PolygonTek/BlueshiftEngine/blob/fbc374cbc391e1147c744649f405a66a27c35d89/Source/Runtime/Private/Platform/Windows/PlatformWinThread.cpp#L27 */
#if (_WIN32_WINNT >= 0x0601)
	int groups = GetActiveProcessorGroupCount();
	int total_processors = 0;
	int group = 0;
	int number = 0;
	int found = 0;
	HANDLE thread = GetCurrentThread();
	GROUP_AFFINITY groupAffinity;

	for (int i = 0; i < groups; i++) {
		int processors = GetActiveProcessorCount(i);
		if (total_processors + processors > logical_cpu) {
			group = i;
			number = logical_cpu - total_processors;
			found = 1;
			break;
		}
		total_processors += processors;
	}
	if (!found) return 0; // logical CPU # too large, does not exist

	memset(&groupAffinity, 0, sizeof(groupAffinity));
	groupAffinity.Group = (WORD) group;
	groupAffinity.Mask = (KAFFINITY) (1ULL << number);
	return SetThreadGroupAffinity(thread, &groupAffinity, NULL);
#else
	if (logical_cpu > (sizeof(DWORD_PTR) * 8)) {
		warnf("set_cpu_affinity for logical CPU %u is not supported in this operating system.\n", logical_cpu);
		return -1;
	}
	HANDLE thread = GetCurrentThread();
	DWORD_PTR threadAffinityMask = 1ULL << logical_cpu;
	return SetThreadAffinityMask(thread, threadAffinityMask);
#endif /* (_WIN32_WINNT >= 0x0601) */
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

INTERNAL_SCOPE cpu_set_t saved_affinity;

static bool save_cpu_affinity(void)
{
	return sched_getaffinity(0, sizeof(saved_affinity), &saved_affinity) == 0;
}

static bool restore_cpu_affinity(void)
{
	return sched_setaffinity(0, sizeof(saved_affinity), &saved_affinity) == 0;
}
#define PRESERVE_CPU_AFFINITY

static bool set_cpu_affinity(logical_cpu_t logical_cpu)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(logical_cpu, &cpuset);
	return sched_setaffinity(0, sizeof(cpuset), &cpuset) == 0;
}
#define SET_CPU_AFFINITY
#endif /* defined linux || defined __linux__ */

#if defined sun || defined __sun
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>

INTERNAL_SCOPE processorid_t saved_binding = PBIND_NONE;

static bool save_cpu_affinity(void)
{
	return processor_bind(P_LWPID, P_MYID, PBIND_QUERY, &saved_binding) == 0;
}

static bool restore_cpu_affinity(void)
{
	return processor_bind(P_LWPID, P_MYID, saved_binding, NULL) == 0;
}
#define PRESERVE_CPU_AFFINITY

static bool set_cpu_affinity(logical_cpu_t logical_cpu)
{
	if (logical_cpu > (sizeof(processorid_t) * 8)) {
		warnf("set_cpu_affinity for logical CPU %u is not supported in this operating system.\n", logical_cpu);
		return -1;
	}
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

INTERNAL_SCOPE cpuset_t saved_affinity;

static bool save_cpu_affinity(void)
{
	return cpuset_getaffinity(CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, sizeof(saved_affinity), &saved_affinity) == 0;
}

static bool restore_cpu_affinity(void)
{
	return cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, sizeof(saved_affinity), &saved_affinity) == 0;
}
#define PRESERVE_CPU_AFFINITY

static bool set_cpu_affinity(logical_cpu_t logical_cpu)
{
	cpuset_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(logical_cpu, &cpuset);
	return cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, sizeof(cpuset), &cpuset) == 0;
}
#define SET_CPU_AFFINITY
#endif /* defined __FreeBSD__ */

#if defined __DragonFly__
#include <pthread.h>
#include <pthread_np.h>

INTERNAL_SCOPE cpuset_t saved_affinity;

static bool save_cpu_affinity(void)
{
	return pthread_getaffinity_np(pthread_self(), sizeof(saved_affinity), &saved_affinity) == 0;
}

static bool restore_cpu_affinity(void)
{
	return pthread_setaffinity_np(pthread_self(), sizeof(saved_affinity), &saved_affinity) == 0;
}
#define PRESERVE_CPU_AFFINITY

static bool set_cpu_affinity(logical_cpu_t logical_cpu)
{
	cpuset_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(logical_cpu, &cpuset);
	return pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0;
}
#define SET_CPU_AFFINITY
#endif /* defined __DragonFly__ */

#if defined __NetBSD__
#include <pthread.h>
#include <sched.h>

INTERNAL_SCOPE cpuset_t *saved_affinity = NULL;

static bool save_cpu_affinity(void)
{
	if (!saved_affinity)
		saved_affinity = cpuset_create();

	return pthread_getaffinity_np(pthread_self(), cpuset_size(saved_affinity), saved_affinity) == 0;
}

static bool restore_cpu_affinity(void)
{
	if (!saved_affinity)
		return false;

	int ret = pthread_setaffinity_np(pthread_self(), cpuset_size(saved_affinity), saved_affinity) == 0;
	cpuset_destroy(saved_affinity);
	saved_affinity = NULL;
	return ret == 0;
}
#define PRESERVE_CPU_AFFINITY

static bool set_cpu_affinity(logical_cpu_t logical_cpu)
{
	cpuset_t *cpuset;
	cpuset = cpuset_create();
	cpuset_set((cpuid_t) logical_cpu, cpuset);
	int ret = pthread_setaffinity_np(pthread_self(), cpuset_size(cpuset), cpuset);
	cpuset_destroy(cpuset);
	return ret == 0;
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

#ifndef PRESERVE_CPU_AFFINITY
static bool save_cpu_affinity(void)
{
	return false;
}

static bool restore_cpu_affinity(void)
{
	return false;
}
#endif /* PRESERVE_CPU_AFFINITY */

#ifndef SET_CPU_AFFINITY
static bool set_cpu_affinity(logical_cpu_t logical_cpu)
{
	static int warning_printed = 0;
	if (!warning_printed) {
		warning_printed = 1;
		warnf("Your system is not supported by libcpuid -- don't know how to set the CPU affinity.\n");
	}
	return false;
}
#endif /* SET_CPU_AFFINITY */

static int cpuid_serialize_raw_data_internal(struct cpu_raw_data_t* single_raw, struct cpu_raw_data_array_t* raw_array, const char* filename)
{
	int i;
	bool end_loop = false;
	const bool use_raw_array = (raw_array != NULL) && raw_array->num_raw > 0;
	logical_cpu_t logical_cpu = 0;
	struct cpu_raw_data_t* raw_ptr = use_raw_array ? &raw_array->raw[0] : single_raw;
	const cpu_architecture_t architecture = cpuid_architecture_identify(raw_ptr);
	FILE *f;

	/* Open file descriptor */
	f = !strcmp(filename, "") ? stdout : fopen(filename, "wt");
	if (!f)
		return cpuid_set_error(ERR_OPEN);
	debugf(1, "Writing raw CPUID dump to '%s'\n", f == stdout ? "stdout" : filename);

	/* Write raw data to output file */
	fprintf(f, "version=%s\n", VERSION);
	while (!end_loop) {
		if (use_raw_array) {
			debugf(2, "Writing raw dump for logical CPU %i\n", logical_cpu);
			fprintf(f, "\n_________________ Logical CPU #%i _________________\n", logical_cpu);
			raw_ptr = &raw_array->raw[logical_cpu];
		}
		switch (architecture) {
			case ARCHITECTURE_X86:
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
				break;
			case ARCHITECTURE_ARM:
				fprintf(f, "arm_midr=%016lx\n", raw_ptr->arm_midr);
				fprintf(f, "arm_mpidr=%016lx\n", raw_ptr->arm_mpidr);
				fprintf(f, "arm_revidr=%016lx\n", raw_ptr->arm_revidr);
				for (i = 0; i < MAX_ARM_ID_AA64DFR_REGS; i++)
					fprintf(f, "arm_id_aa64dfr%d=%016lx\n", i, raw_ptr->arm_id_aa64dfr[i]);
				for (i = 0; i < MAX_ARM_ID_AA64ISAR_REGS; i++)
					fprintf(f, "arm_id_aa64isar%d=%016lx\n", i, raw_ptr->arm_id_aa64isar[i]);
				for (i = 0; i < MAX_ARM_ID_AA64MMFR_REGS; i++)
					fprintf(f, "arm_id_aa64mmfr%d=%016lx\n", i, raw_ptr->arm_id_aa64mmfr[i]);
				for (i = 0; i < MAX_ARM_ID_AA64PFR_REGS; i++)
					fprintf(f, "arm_id_aa64pfr%d=%016lx\n", i, raw_ptr->arm_id_aa64pfr[i]);
				for (i = 0; i < MAX_ARM_ID_AA64SMFR_REGS; i++)
					fprintf(f, "arm_id_aa64smfr%d=%016lx\n", i, raw_ptr->arm_id_aa64smfr[i]);
				for (i = 0; i < MAX_ARM_ID_AA64ZFR_REGS; i++)
					fprintf(f, "arm_id_aa64zfr%d=%016lx\n", i, raw_ptr->arm_id_aa64zfr[i]);
				break;
			default:
				break;
		}

		logical_cpu++;
		end_loop = ((use_raw_array && (logical_cpu >= raw_array->num_raw)) || !use_raw_array);
	}

	/* Close file descriptor */
	if (strcmp(filename, ""))
		fclose(f);
	return cpuid_set_error(ERR_OK);
}

#define RAW_ASSIGN_LINE_X86(__line) __line[EAX] = eax ; __line[EBX] = ebx ; __line[ECX] = ecx ; __line[EDX] = edx
#define RAW_ASSIGN_LINE_ARM(__line) __line = arm_reg
static int cpuid_deserialize_raw_data_internal(struct cpu_raw_data_t* single_raw, struct cpu_raw_data_array_t* raw_array, const char* filename)
{
	int i;
	int cur_line = 0;
	int assigned = 0;
	int subleaf = 0;
	bool is_header = true;
	bool is_libcpuid_dump = true;
	bool is_aida64_dump = false;
	const bool use_raw_array = (raw_array != NULL);
	logical_cpu_t logical_cpu = 0;
	uint32_t addr, eax, ebx, ecx, edx;
	uint64_t arm_reg;
	char version[8] = "";
	char line[100];
	struct cpu_raw_data_t* raw_ptr = single_raw;
	FILE *f;

	/* Open file descriptor */
	f = !strcmp(filename, "") ? stdin : fopen(filename, "rt");
	if (!f)
		return cpuid_set_error(ERR_OPEN);
	debugf(1, "Opening raw dump from '%s'\n", f == stdin ? "stdin" : filename);

	if (use_raw_array)
		cpu_raw_data_array_t_constructor(raw_array, false);

	/* Parse file and store data in cpu_raw_data_t */
	while (fgets(line, sizeof(line), f) != NULL) {
		i = -1;
		line[strcspn(line, "\n")] = '\0';
		if (line[0] == '\0') // Skip empty lines
			continue;
		if (!strcmp(line, "--------------------------------------------------------------------------------")) // Skip test results
			break;
		cur_line++;
		if (is_header) {
			if (sscanf(line, "version=%s", version) >= 1) {
				debugf(2, "Recognized version '%s' from raw dump\n", version);
				is_libcpuid_dump = true;
				is_aida64_dump = false;
				continue;
			}
			else if (sscanf(line, "basic_cpuid[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) {
				debugf(2, "Parsing raw dump for a single CPU dump\n");
				is_header = false;
				is_libcpuid_dump = true;
				is_aida64_dump = false;
				if (use_raw_array) {
					cpuid_grow_raw_data_array(raw_array, 1);
					raw_ptr = &raw_array->raw[0];
					raw_array->with_affinity = false;
				}
			}
			else if (!strcmp(line, "------[ Versions ]------") ||
			         !strcmp(line, "------[ Logical CPU #0 ]------") ||
			         !strcmp(line, "------[ CPUID Registers / Logical CPU #0 ]------") ||
			         strstr(line, "CPU#000 AffMask: 0x")) {
				debugf(2, "Recognized AIDA64 raw dump\n");
				is_header = false;
				is_libcpuid_dump = false;
				is_aida64_dump = true;
			}
		}

		if (is_libcpuid_dump) {
			if (use_raw_array && (sscanf(line, "_________________ Logical CPU #%hi _________________", &logical_cpu) >= 1)) {
				debugf(2, "Parsing raw dump for logical CPU %i\n", logical_cpu);
				is_header = false;
				cpuid_grow_raw_data_array(raw_array, logical_cpu + 1);
				raw_ptr = &raw_array->raw[logical_cpu];
				raw_array->with_affinity = true;
			}
			else if ((sscanf(line, "basic_cpuid[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_CPUID_LEVEL)) {
				RAW_ASSIGN_LINE_X86(raw_ptr->basic_cpuid[i]);
			}
			else if ((sscanf(line, "ext_cpuid[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_EXT_CPUID_LEVEL)) {
				RAW_ASSIGN_LINE_X86(raw_ptr->ext_cpuid[i]);
			}
			else if ((sscanf(line, "intel_fn4[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_INTELFN4_LEVEL)) {
				RAW_ASSIGN_LINE_X86(raw_ptr->intel_fn4[i]);
			}
			else if ((sscanf(line, "intel_fn11[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_INTELFN11_LEVEL)) {
				RAW_ASSIGN_LINE_X86(raw_ptr->intel_fn11[i]);
			}
			else if ((sscanf(line, "intel_fn12h[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_INTELFN12H_LEVEL)) {
				RAW_ASSIGN_LINE_X86(raw_ptr->intel_fn12h[i]);
			}
			else if ((sscanf(line, "intel_fn14h[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_INTELFN14H_LEVEL)) {
				RAW_ASSIGN_LINE_X86(raw_ptr->intel_fn14h[i]);
			}
			else if ((sscanf(line, "amd_fn8000001dh[%d]=%x %x %x %x", &i, &eax, &ebx, &ecx, &edx) >= 5) && (i >= 0) && (i < MAX_AMDFN8000001DH_LEVEL)) {
				RAW_ASSIGN_LINE_X86(raw_ptr->amd_fn8000001dh[i]);
			}
			else if ((sscanf(line, "arm_midr=%lx", &arm_reg) >= 1)) {
				RAW_ASSIGN_LINE_ARM(raw_ptr->arm_midr);
			}
			else if ((sscanf(line, "arm_mpidr=%lx", &arm_reg) >= 1)) {
				RAW_ASSIGN_LINE_ARM(raw_ptr->arm_mpidr);
			}
			else if ((sscanf(line, "arm_revidr=%lx", &arm_reg) >= 1)) {
				RAW_ASSIGN_LINE_ARM(raw_ptr->arm_revidr);
			}
			else if ((sscanf(line, "arm_id_aa64dfr%d=%lx", &i, &arm_reg) >= 2)) {
				RAW_ASSIGN_LINE_ARM(raw_ptr->arm_id_aa64dfr[i]);
			}
			else if ((sscanf(line, "arm_id_aa64isar%d=%lx", &i, &arm_reg) >= 2)) {
				RAW_ASSIGN_LINE_ARM(raw_ptr->arm_id_aa64isar[i]);
			}
			else if ((sscanf(line, "arm_id_aa64mmfr%d=%lx", &i, &arm_reg) >= 2)) {
				RAW_ASSIGN_LINE_ARM(raw_ptr->arm_id_aa64mmfr[i]);
			}
			else if ((sscanf(line, "arm_id_aa64pfr%d=%lx", &i, &arm_reg) >= 2)) {
				RAW_ASSIGN_LINE_ARM(raw_ptr->arm_id_aa64pfr[i]);
			}
			else if ((sscanf(line, "arm_id_aa64smfr%d=%lx", &i, &arm_reg) >= 2)) {
				RAW_ASSIGN_LINE_ARM(raw_ptr->arm_id_aa64smfr[i]);
			}
			else if ((sscanf(line, "arm_id_aa64zfr%d=%lx", &i, &arm_reg) >= 2)) {
				RAW_ASSIGN_LINE_ARM(raw_ptr->arm_id_aa64zfr[i]);
			}
			else if (line[0] != '\0') {
				warnf("Warning: file '%s', line %d: '%s' not understood!\n", filename, cur_line, line);
			}
		}
		else if (is_aida64_dump) {
			if (use_raw_array && ((sscanf(line, "------[ Logical CPU #%hi ]------", &logical_cpu) >= 1) ||
			                      (sscanf(line, "------[ CPUID Registers / Logical CPU #%hi ]------", &logical_cpu) >= 1) ||
			                      (sscanf(line, "CPU#%hi AffMask: 0x%*x", &logical_cpu) >= 1))) {
				debugf(2, "Parsing AIDA64 raw dump for logical CPU %i\n", logical_cpu);
				cpuid_grow_raw_data_array(raw_array, logical_cpu + 1);
				raw_ptr = &raw_array->raw[logical_cpu];
				raw_array->with_affinity = true;
				continue;
			}
			subleaf = 0;
			assigned = sscanf(line, "CPUID %x: %x-%x-%x-%x [SL %02i]", &addr, &eax, &ebx, &ecx, &edx, &subleaf);
			debugf(3, "raw line %d: %i items assigned for string '%s'\n", cur_line, assigned, line);
			if ((assigned >= 5) && (subleaf == 0)) {
				if (addr < MAX_CPUID_LEVEL) {
					i = (int) addr;
					RAW_ASSIGN_LINE_X86(raw_ptr->basic_cpuid[i]);
				}
				else if ((addr >= ADDRESS_EXT_CPUID_START) && (addr < ADDRESS_EXT_CPUID_END)) {
					i = (int) addr - ADDRESS_EXT_CPUID_START;
					RAW_ASSIGN_LINE_X86(raw_ptr->ext_cpuid[i]);
				}
			}
			if (assigned >= 6) {
				i = subleaf;
				switch (addr) {
					case 0x00000004: RAW_ASSIGN_LINE_X86(raw_ptr->intel_fn4[i]);       break;
					case 0x0000000B: RAW_ASSIGN_LINE_X86(raw_ptr->intel_fn11[i]);      break;
					case 0x00000012: RAW_ASSIGN_LINE_X86(raw_ptr->intel_fn12h[i]);     break;
					case 0x00000014: RAW_ASSIGN_LINE_X86(raw_ptr->intel_fn14h[i]);     break;
					case 0x8000001D: RAW_ASSIGN_LINE_X86(raw_ptr->amd_fn8000001dh[i]); break;
					default: break;
				}
			}
		}
	}

	/* Close file descriptor */
	if (strcmp(filename, ""))
		fclose(f);
	return cpuid_set_error(ERR_OK);
}
#undef RAW_ASSIGN_LINE_X86
#undef RAW_ASSIGN_LINE_ARM

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
		{ 31, CPU_FEATURE_HYPERVISOR },
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
		return cpuid_set_error(ERR_CPU_UNKN);

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
	ext = raw->ext_cpuid[0][EAX] - 0x80000000;

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
	return cpuid_set_error(ERR_OK);
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
		cpuid_set_error(ERR_NO_MEM);
		return;
	}
	list->num_entries = n;
	last = -1;
	n = 0;
	for (i = 0; i <= l; i++) if (i == l || csv[i] == ',') {
		list->names[n] = (char*) malloc(i - last);
		if (!list->names[n]) { /* Memory allocation failed */
			cpuid_set_error(ERR_NO_MEM);
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

static bool cpu_ident_id_x86(struct cpu_raw_data_t* raw, struct internal_topology_t* topology)
{
	bool is_apic_id_supported = false;
	uint8_t subleaf;
	uint8_t level_type = 0;
	uint8_t mask_core_shift = 0;
	uint32_t mask_smt_shift, core_plus_mask_width, package_mask, core_mask, smt_mask = 0;
	char vendor_str[VENDOR_STR_MAX];

	/* Only AMD and Intel x86 CPUs support Extended Processor Topology Eumeration */
	const cpu_vendor_t vendor = cpuid_vendor_identify(raw->basic_cpuid[0], vendor_str);
	switch (vendor) {
		case VENDOR_INTEL:
		case VENDOR_AMD:
			is_apic_id_supported = true;
			break;
		case VENDOR_UNKNOWN:
			cpuid_set_error(ERR_CPU_UNKN);
			/* Fall through */
		default:
			is_apic_id_supported = false;
			break;
	}

	/* Documentation: Intel® 64 and IA-32 Architectures Software Developer’s Manual
	   Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D and 4
	   https://cdrdv2.intel.com/v1/dl/getContent/671200
	   Examples 8-20 and 8-22 are implemented below.
	*/

	/* Check if leaf 0Bh is supported and if number of logical processors at this level type is greater than 0 */
	if (!is_apic_id_supported || (raw->basic_cpuid[0][EAX] < 11) || (EXTRACTS_BITS(raw->basic_cpuid[11][EBX], 15, 0) == 0)) {
		warnf("Warning: APIC ID are not supported, core count can be wrong if SMT is disabled and cache instances count will not be available.\n");
		return false;
	}

	/* Derive core mask offsets */
	for (subleaf = 0; (raw->intel_fn11[subleaf][EAX] != 0x0) && (raw->intel_fn11[subleaf][EBX] != 0x0) && (subleaf < MAX_INTELFN11_LEVEL); subleaf++)
		mask_core_shift = EXTRACTS_BITS(raw->intel_fn11[subleaf][EAX], 4, 0);

	/* Find mask and ID for SMT and cores */
	for (subleaf = 0; (raw->intel_fn11[subleaf][EAX] != 0x0) && (raw->intel_fn11[subleaf][EBX] != 0x0) && (subleaf < MAX_INTELFN11_LEVEL); subleaf++) {
		level_type        = EXTRACTS_BITS(raw->intel_fn11[subleaf][ECX], 15, 8);
		topology->apic_id = raw->intel_fn11[subleaf][EDX];
		switch (level_type) {
			case 0x01:
				mask_smt_shift    = EXTRACTS_BITS(raw->intel_fn11[subleaf][EAX], 4, 0);
				smt_mask          = ~( (-1) << mask_smt_shift);
				topology->smt_id  = topology->apic_id & smt_mask;
				break;
			case 0x02:
				core_plus_mask_width = ~( (-1) << mask_core_shift);
				core_mask            = core_plus_mask_width ^ smt_mask;
				topology->core_id    = topology->apic_id & core_mask;
				break;
			default:
				break;
		}
	}

	/* Find mask and ID for packages */
	package_mask          = (-1) << mask_core_shift;
	topology->package_id  = topology->apic_id & package_mask;

	return (level_type > 0);
}

static bool cpu_ident_id_arm(struct cpu_raw_data_t* raw, struct internal_topology_t* topology)
{
	/* Documentation: Multiprocessor Affinity Register
	   https://developer.arm.com/documentation/ddi0601/2020-12/AArch64-Registers/MPIDR-EL1--Multiprocessor-Affinity-Register
	*/
	const bool aff0_is_threads = EXTRACTS_BIT(raw->arm_mpidr, 24);
	if (aff0_is_threads) {
		/* Aff0: the level identifies individual threads within a multithreaded core
		   On single-threaded CPUs this field has the value 0x00 */
		topology->smt_id     = EXTRACTS_BITS(raw->arm_mpidr,  7,  0); // Aff0
		topology->core_id    = EXTRACTS_BITS(raw->arm_mpidr, 15,  8); // Aff1
		topology->package_id = EXTRACTS_BITS(raw->arm_mpidr, 23, 16); // Aff2
	}
	else {
		topology->core_id    = EXTRACTS_BITS(raw->arm_mpidr,  7,  0); // Aff0
		topology->package_id = EXTRACTS_BITS(raw->arm_mpidr, 15,  8); // Aff1
	}

	/* Always implemented since ARMv7
	   https://developer.arm.com/documentation/ddi0406/c/System-Level-Architecture/System-Control-Registers-in-a-PMSA-implementation/PMSA-System-control-registers-descriptions--in-register-order/MPIDR--Multiprocessor-Affinity-Register--PMSA?lang=en
	*/
	return true;
}

static bool cpu_ident_id(logical_cpu_t logical_cpu, struct cpu_raw_data_t* raw, struct internal_topology_t* topology)
{
	topology_t_constructor(topology, logical_cpu);

	const cpu_architecture_t architecture = cpuid_architecture_identify(raw);
	switch (architecture) {
		case ARCHITECTURE_X86:
			return cpu_ident_id_x86(raw, topology);
		case ARCHITECTURE_ARM:
			return cpu_ident_id_arm(raw, topology);
		default:
			break;
	}

	return false;
}

/* Interface: */

int cpuid_get_total_cpus(void)
{
	return get_total_cpus();
}

int cpuid_present(void)
{
#if defined(PLATFORM_X86) || defined(PLATFORM_X64)
	return cpuid_exists_by_eflags();
#elif defined(PLATFORM_AARCH64)
# if defined(HAVE_GETAUXVAL) /* Linux */
	return (getauxval(AT_HWCAP) & HWCAP_CPUID);
# elif defined(HAVE_ELF_AUX_INFO) /* FreeBSD */
	unsigned long hwcap = 0;
	if (elf_aux_info(AT_HWCAP, &hwcap, sizeof(hwcap)) == 0)
		return ((hwcap & HWCAP_CPUID) != 0);
# endif /* HAVE_GETAUXVAL */
	/* On AArch64, return 0 by default */
	return 0;
#else
	return 0;
#endif
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
	if (!cpuid_present())
		return cpuid_set_error(ERR_NO_CPUID);
#if defined(PLATFORM_X86) || defined(PLATFORM_X64)
	unsigned i;
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
#elif defined(PLATFORM_ARM)
	/* We cannot support ARM CPUs running in 32-bit mode, because the Main ID Register is accessible only in privileged modes
	   Some related links:
	   - https://github.com/anrieff/libcpuid/issues/96
	   - https://developer.arm.com/documentation/ddi0406/b/System-Level-Architecture/Protected-Memory-System-Architecture--PMSA-/CP15-registers-for-a-PMSA-implementation/c0--Main-ID-Register--MIDR-
	*/
# warning The 32-bit ARM platform is not supported (Main ID Register is accessible only in privileged modes)
	UNUSED(data);
#elif defined(PLATFORM_AARCH64)
	cpu_exec_mrs("MIDR_EL1", data->arm_midr);
	cpu_exec_mrs("MPIDR_EL1", data->arm_mpidr);
	cpu_exec_mrs("REVIDR_EL1", data->arm_revidr);
	cpu_exec_mrs(SYS_ID_AA64DFR0_EL1, data->arm_id_aa64dfr[0]);
	cpu_exec_mrs(SYS_ID_AA64DFR1_EL1, data->arm_id_aa64dfr[1]);
	cpu_exec_mrs(SYS_ID_AA64ISAR0_EL1, data->arm_id_aa64isar[0]);
	cpu_exec_mrs(SYS_ID_AA64ISAR1_EL1, data->arm_id_aa64isar[1]);
	cpu_exec_mrs(SYS_ID_AA64ISAR2_EL1, data->arm_id_aa64isar[2]);
	cpu_exec_mrs(SYS_ID_AA64MMFR0_EL1, data->arm_id_aa64mmfr[0]);
	cpu_exec_mrs(SYS_ID_AA64MMFR1_EL1, data->arm_id_aa64mmfr[1]);
	cpu_exec_mrs(SYS_ID_AA64MMFR2_EL1, data->arm_id_aa64mmfr[2]);
	cpu_exec_mrs(SYS_ID_AA64MMFR3_EL1, data->arm_id_aa64mmfr[3]);
	cpu_exec_mrs(SYS_ID_AA64MMFR4_EL1, data->arm_id_aa64mmfr[4]);
	cpu_exec_mrs(SYS_ID_AA64PFR0_EL1, data->arm_id_aa64pfr[0]);
	cpu_exec_mrs(SYS_ID_AA64PFR1_EL1, data->arm_id_aa64pfr[1]);
	cpu_exec_mrs(SYS_ID_AA64PFR2_EL1, data->arm_id_aa64pfr[2]);
	cpu_exec_mrs(SYS_ID_AA64SMFR0_EL1, data->arm_id_aa64smfr[0]);
	cpu_exec_mrs(SYS_ID_AA64ZFR0_EL1, data->arm_id_aa64zfr[0]);
#endif
	return cpuid_set_error(ERR_OK);
}

int cpuid_get_all_raw_data(struct cpu_raw_data_array_t* data)
{
	int cur_error = cpuid_set_error(ERR_OK);
	int ret_error = cpuid_set_error(ERR_OK);
	logical_cpu_t logical_cpu = 0;
	struct cpu_raw_data_t* raw_ptr = NULL;

	if (data == NULL)
		return cpuid_set_error(ERR_HANDLE);

	bool affinity_saved = save_cpu_affinity();

	cpu_raw_data_array_t_constructor(data, true);
	while (set_cpu_affinity(logical_cpu) || logical_cpu == 0) {
		debugf(2, "Getting raw dump for logical CPU %i\n", logical_cpu);
		cpuid_grow_raw_data_array(data, logical_cpu + 1);
		raw_ptr = &data->raw[logical_cpu];
		cur_error = cpuid_get_raw_data(raw_ptr);
		if (ret_error == ERR_OK)
			ret_error = cur_error;
		logical_cpu++;
	}

	if (affinity_saved)
		restore_cpu_affinity();

	return ret_error;
}

int cpuid_serialize_raw_data(struct cpu_raw_data_t* data, const char* filename)
{
	return cpuid_serialize_raw_data_internal(data, NULL, filename);
}

int cpuid_serialize_all_raw_data(struct cpu_raw_data_array_t* data, const char* filename)
{
	return cpuid_serialize_raw_data_internal(NULL, data, filename);
}

int cpuid_deserialize_raw_data(struct cpu_raw_data_t* data, const char* filename)
{
	raw_data_t_constructor(data);
	return cpuid_deserialize_raw_data_internal(data, NULL, filename);
}

int cpuid_deserialize_all_raw_data(struct cpu_raw_data_array_t* data, const char* filename)
{
	return cpuid_deserialize_raw_data_internal(NULL, data, filename);
}

int cpu_ident_internal(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct internal_id_info_t* internal)
{
	int r;
	struct cpu_raw_data_t myraw;
	if (!raw) {
		if ((r = cpuid_get_raw_data(&myraw)) < 0)
			return cpuid_set_error(r);
		raw = &myraw;
	}
	cpu_id_t_constructor(data);
	memset(internal->cache_mask, 0, sizeof(internal->cache_mask));
	data->architecture = cpuid_architecture_identify(raw);

	switch (data->architecture) {
		case ARCHITECTURE_X86:
			if ((r = cpuid_basic_identify(raw, data)) < 0)
				return cpuid_set_error(r);
			switch (data->vendor) {
				case VENDOR_INTEL:
					r = cpuid_identify_intel(raw, data, internal);
					break;
				case VENDOR_AMD:
				case VENDOR_HYGON:
					r = cpuid_identify_amd(raw, data, internal);
					break;
				case VENDOR_CENTAUR:
					r = cpuid_identify_centaur(raw, data, internal);
					break;
				default:
					break;
			}
			break;
		case ARCHITECTURE_ARM:
			r = cpuid_identify_arm(raw, data);
			break;
		default:
			return cpuid_set_error(ERR_CPU_UNKN);
	}
	/* Backward compatibility */
	/* - Deprecated since v0.5.0 */
	data->l1_assoc     = data->l1_data_assoc;
	data->l1_cacheline = data->l1_data_cacheline;
	return cpuid_set_error(r);
}

static cpu_purpose_t cpu_ident_purpose(struct cpu_raw_data_t* raw)
{
	cpu_vendor_t vendor = VENDOR_UNKNOWN;
	cpu_purpose_t purpose = PURPOSE_GENERAL;
	char vendor_str[VENDOR_STR_MAX];

	const cpu_architecture_t architecture = cpuid_architecture_identify(raw);
	switch (architecture) {
		case ARCHITECTURE_X86:
			vendor = cpuid_vendor_identify(raw->basic_cpuid[0], vendor_str);
			switch (vendor) {
				case VENDOR_AMD:
					purpose = cpuid_identify_purpose_amd(raw);
					break;
				case VENDOR_INTEL:
					purpose = cpuid_identify_purpose_intel(raw);
					break;
				default:
					break;
			}
			break;
		case ARCHITECTURE_ARM:
			purpose = cpuid_identify_purpose_arm(raw);
			break;
		default:
			break;
	}

	debugf(3, "Identified a '%s' CPU core type\n", cpu_purpose_str(purpose));

	return purpose;
}

int cpu_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int r;
	struct internal_id_info_t throwaway;
	r = cpu_ident_internal(raw, data, &throwaway);
	return r;
}

static void update_core_instances(struct internal_core_instances_t* cores,
                                   struct internal_topology_t* topology)
{
	uint32_t core_id_index = 0;

	core_id_index = topology->core_id % CORES_HTABLE_SIZE;
	if ((cores->htable[core_id_index].core_id == 0) || (cores->htable[core_id_index].core_id == topology->core_id)) {
		if (cores->htable[core_id_index].num_logical_cpu == 0)
			cores->instances++;
		cores->htable[core_id_index].core_id = topology->core_id;
		cores->htable[core_id_index].num_logical_cpu++;
	}
	else {
		warnf("update_core_instances: collision at index %u (core ID is %i, not %i)\n",
			core_id_index, cores->htable[core_id_index].core_id, topology->core_id);
	}
}

static void update_cache_instances(struct internal_cache_instances_t* caches,
                                   struct internal_topology_t* topology,
                                   struct internal_id_info_t* id_info,
                                   bool debugf_is_needed)
{
	uint32_t cache_id_index = 0;
	cache_type_t level;

	for (level = 0; level < NUM_CACHE_TYPES; level++) {
		if (id_info->cache_mask[level] == 0x00000000) {
			topology->cache_id[level] = -1;
			continue;
		}
		topology->cache_id[level] = topology->apic_id & id_info->cache_mask[level];
		cache_id_index             = topology->cache_id[level] % CACHES_HTABLE_SIZE;
		if ((caches->htable[level][cache_id_index].cache_id == 0) || (caches->htable[level][cache_id_index].cache_id == topology->cache_id[level])) {
			if (caches->htable[level][cache_id_index].num_logical_cpu == 0)
				caches->instances[level]++;
			caches->htable[level][cache_id_index].cache_id = topology->cache_id[level];
			caches->htable[level][cache_id_index].num_logical_cpu++;
		}
		else {
			warnf("update_cache_instances: collision at index %u (cache ID is %i, not %i)\n",
				cache_id_index, caches->htable[level][cache_id_index].cache_id, topology->cache_id[level]);
		}
	}

	if (debugf_is_needed)
		debugf(3, "Logical CPU %4u: APIC ID %4i, package ID %4i, core ID %4i, thread %i, L1I$ ID %4i, L1D$ ID %4i, L2$ ID %4i, L3$ ID %4i, L4$ ID %4i\n",
			topology->logical_cpu, topology->apic_id, topology->package_id, topology->core_id, topology->smt_id,
			topology->cache_id[L1I], topology->cache_id[L1D], topology->cache_id[L2], topology->cache_id[L3], topology->cache_id[L4]);
}

int cpu_identify_all(struct cpu_raw_data_array_t* raw_array, struct system_id_t* system)
{
	int cur_error = cpuid_set_error(ERR_OK);
	int ret_error = cpuid_set_error(ERR_OK);
	double smt_divisor;
	bool is_smt_supported;
	bool is_topology_supported = true;
	int16_t cpu_type_index = -1;
	int32_t cur_package_id = 0;
	logical_cpu_t logical_cpu = 0;
	cpu_purpose_t purpose;
	cpu_affinity_mask_t affinity_mask;
	struct cpu_raw_data_array_t my_raw_array;
	struct internal_topology_t topology;
	struct internal_type_info_array_t type_info;
	struct internal_cache_instances_t caches_all;

	/* Init variables */
	if (system == NULL)
		return cpuid_set_error(ERR_HANDLE);
	if (!raw_array) {
		if ((ret_error = cpuid_get_all_raw_data(&my_raw_array)) < 0)
			return cpuid_set_error(ret_error);
		raw_array = &my_raw_array;
	}
	system_id_t_constructor(system);
	type_info_array_t_constructor(&type_info);
	cache_instances_t_constructor(&caches_all);
	if (raw_array->with_affinity)
		init_affinity_mask(&affinity_mask);

	/* Iterate over all raw */
	for (logical_cpu = 0; logical_cpu < raw_array->num_raw; logical_cpu++) {
		debugf(2, "Identifying logical core %u\n", logical_cpu);
		/* Get CPU purpose and APIC ID
		   For hybrid CPUs, the purpose may be different than the previous iteration (e.g. from P-cores to E-cores)
		   APIC ID are unique for each logical CPU cores.
		*/
		purpose = cpu_ident_purpose(&raw_array->raw[logical_cpu]);
		if (raw_array->with_affinity && is_topology_supported) {
			is_topology_supported = cpu_ident_id(logical_cpu, &raw_array->raw[logical_cpu], &topology);
			if (is_topology_supported)
				cur_package_id = topology.package_id;
		}

		/* Put data to system->cpu_types on the first iteration or when purpose is new.
		   For motherboards with multiple CPUs, we grow the array when package ID is different. */
		cpu_type_index = cpuid_find_index_system_id(system, purpose, &type_info, cur_package_id, is_topology_supported);
		if (cpu_type_index < 0) {
			cpu_type_index = system->num_cpu_types;
			cpuid_grow_system_id(system, system->num_cpu_types + 1);
			cpuid_grow_type_info(&type_info, type_info.num + 1);
			cur_error = cpu_ident_internal(&raw_array->raw[logical_cpu], &system->cpu_types[cpu_type_index], &type_info.data[cpu_type_index].id_info);
			type_info.data[cpu_type_index].purpose = purpose;
			if (ret_error == ERR_OK)
				ret_error = cur_error;
			if (is_topology_supported)
				type_info.data[cpu_type_index].package_id = cur_package_id;
			if (raw_array->with_affinity)
				system->cpu_types[cpu_type_index].num_logical_cpus = 0;
		}

		/* Increment counters */
		if (raw_array->with_affinity) {
			set_affinity_mask_bit(logical_cpu, &system->cpu_types[cpu_type_index].affinity_mask);
			system->cpu_types[cpu_type_index].num_logical_cpus++;
			if (is_topology_supported) {
				update_core_instances(&type_info.data[cpu_type_index].core_instances, &topology);
				update_cache_instances(&type_info.data[cpu_type_index].cache_instances, &topology, &type_info.data[cpu_type_index].id_info, true);
				update_cache_instances(&caches_all,  &topology, &type_info.data[cpu_type_index].id_info, false);
			}
		}
	}

	/* Update counters for all CPU types */
	for (cpu_type_index = 0; cpu_type_index < system->num_cpu_types; cpu_type_index++) {
		/* Overwrite core and cache counters when information is available per core */
		if (raw_array->with_affinity) {
			if (is_topology_supported) {
				system->cpu_types[cpu_type_index].num_cores                = type_info.data[cpu_type_index].core_instances.instances;
				system->cpu_types[cpu_type_index].l1_instruction_instances = type_info.data[cpu_type_index].cache_instances.instances[L1I];
				system->cpu_types[cpu_type_index].l1_data_instances        = type_info.data[cpu_type_index].cache_instances.instances[L1D];
				system->cpu_types[cpu_type_index].l2_instances             = type_info.data[cpu_type_index].cache_instances.instances[L2];
				system->cpu_types[cpu_type_index].l3_instances             = type_info.data[cpu_type_index].cache_instances.instances[L3];
				system->cpu_types[cpu_type_index].l4_instances             = type_info.data[cpu_type_index].cache_instances.instances[L4];
			}
			else {
				/* Note: if SMT is disabled by BIOS, smt_divisor will no reflect the current state properly */
				is_smt_supported = system->cpu_types[cpu_type_index].num_cores > 0 ? (system->cpu_types[cpu_type_index].num_logical_cpus % system->cpu_types[cpu_type_index].num_cores) == 0 : false;
				smt_divisor      = is_smt_supported ? system->cpu_types[cpu_type_index].num_logical_cpus / system->cpu_types[cpu_type_index].num_cores : 1.0;
				system->cpu_types[cpu_type_index].num_cores = (int32_t) (system->cpu_types[cpu_type_index].num_logical_cpus / smt_divisor);
			}
		}

		/* Update the total_logical_cpus value for each purpose */
		system->cpu_types[cpu_type_index].total_logical_cpus = logical_cpu;
	}
	cpuid_free_type_info(&type_info);

	/* Update the grand total of cache instances */
	if (is_topology_supported) {
		system->l1_instruction_total_instances = caches_all.instances[L1I];
		system->l1_data_total_instances        = caches_all.instances[L1D];
		system->l2_total_instances             = caches_all.instances[L2];
		system->l3_total_instances             = caches_all.instances[L3];
		system->l4_total_instances             = caches_all.instances[L4];
	}

	return ret_error;
}

int cpu_request_core_type(cpu_purpose_t purpose, struct cpu_raw_data_array_t* raw_array, struct cpu_id_t* data)
{
	int error;
	logical_cpu_t logical_cpu = 0;
	struct cpu_raw_data_array_t my_raw_array;
	struct internal_id_info_t throwaway;

	if (!raw_array) {
		if ((error = cpuid_get_all_raw_data(&my_raw_array)) < 0)
			return cpuid_set_error(error);
		raw_array = &my_raw_array;
	}

	for (logical_cpu = 0; logical_cpu < raw_array->num_raw; logical_cpu++) {
		if (cpu_ident_purpose(&raw_array->raw[logical_cpu]) == purpose) {
			cpu_ident_internal(&raw_array->raw[logical_cpu], data, &throwaway);
			return cpuid_set_error(ERR_OK);
		}
	}

	return cpuid_set_error(ERR_NOT_FOUND);
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
		{ PURPOSE_GENERAL,       "general"              },
		{ PURPOSE_PERFORMANCE,   "performance"          },
		{ PURPOSE_EFFICIENCY,    "efficiency"           },
		{ PURPOSE_LP_EFFICIENCY, "low-power efficiency" },
		{ PURPOSE_U_PERFORMANCE, "ultimate performance" },
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

char* affinity_mask_str_r(cpu_affinity_mask_t* affinity_mask, char* buffer, uint32_t buffer_len)
{
	logical_cpu_t mask_index = __MASK_SETSIZE - 1;
	logical_cpu_t str_index = 0;
	bool do_print = false;

	while (((uint32_t) str_index) + 1 < buffer_len) {
		if (do_print || (mask_index < 4) || (affinity_mask->__bits[mask_index] != 0x00)) {
			snprintf(&buffer[str_index], 3, "%02X", affinity_mask->__bits[mask_index]);
			do_print = true;
			str_index += 2;
		}
		/* mask_index in unsigned, so we cannot decrement it beyond 0 */
		if (mask_index == 0)
			break;
		mask_index--;
	}
	buffer[str_index] = '\0';

	return buffer;
}

char* affinity_mask_str(cpu_affinity_mask_t* affinity_mask)
{
	static char buffer[__MASK_SETSIZE + 1] = "";
	return affinity_mask_str_r(affinity_mask, buffer, __MASK_SETSIZE + 1);
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
		{ CPU_FEATURE_HYPERVISOR, "hypervisor" },
		{ CPU_FEATURE_ASID16, "asid16" },
		{ CPU_FEATURE_ADVSIMD, "advsimd" },
		{ CPU_FEATURE_CRC32, "crc32" },
		{ CPU_FEATURE_CSV2_1P1, "csv2_1p1" },
		{ CPU_FEATURE_CSV2_1P2, "csv2_1p2" },
		{ CPU_FEATURE_CSV2_2, "csv2_2" },
		{ CPU_FEATURE_CSV2_3, "csv2_3" },
		{ CPU_FEATURE_DOUBLELOCK, "doublelock" },
		{ CPU_FEATURE_ETS2, "ets2" },
		{ CPU_FEATURE_FP, "fp" },
		{ CPU_FEATURE_MIXEDEND, "mixedend" },
		{ CPU_FEATURE_PMULL, "pmull" },
		{ CPU_FEATURE_PMUV3, "pmuv3" },
		{ CPU_FEATURE_SHA1, "sha1" },
		{ CPU_FEATURE_SHA256, "sha256" },
		{ CPU_FEATURE_HAFDBS, "hafdbs" },
		{ CPU_FEATURE_HPDS, "hpds" },
		{ CPU_FEATURE_LOR, "lor" },
		{ CPU_FEATURE_LSE, "lse" },
		{ CPU_FEATURE_PAN, "pan" },
		{ CPU_FEATURE_PMUV3P1, "pmuv3p1" },
		{ CPU_FEATURE_RDM, "rdm" },
		{ CPU_FEATURE_VHE, "vhe" },
		{ CPU_FEATURE_VMID16, "vmid16" },
		//{ CPU_FEATURE_AA32HPD, "aa32hpd" },
		//{ CPU_FEATURE_AA32I8MM, "aa32i8mm" },
		{ CPU_FEATURE_DPB, "dpb" },
		{ CPU_FEATURE_DEBUGV8P2, "debugv8p2" },
		{ CPU_FEATURE_F32MM, "f32mm" },
		{ CPU_FEATURE_F64MM, "f64mm" },
		{ CPU_FEATURE_FP16, "fp16" },
		{ CPU_FEATURE_HPDS2, "hpds2" },
		{ CPU_FEATURE_I8MM, "i8mm" },
		{ CPU_FEATURE_IESB, "iesb" },
		{ CPU_FEATURE_LPA, "lpa" },
		{ CPU_FEATURE_LSMAOC, "lsmaoc" },
		{ CPU_FEATURE_LVA, "lva" },
		{ CPU_FEATURE_PAN2, "pan2" },
		{ CPU_FEATURE_RAS, "ras" },
		{ CPU_FEATURE_SHA3, "sha3" },
		{ CPU_FEATURE_SHA512, "sha512" },
		{ CPU_FEATURE_SM3, "sm3" },
		{ CPU_FEATURE_SM4, "sm4" },
		{ CPU_FEATURE_SPE, "spe" },
		{ CPU_FEATURE_SVE, "sve" },
		{ CPU_FEATURE_TTCNP, "ttcnp" },
		{ CPU_FEATURE_UAO, "uao" },
		{ CPU_FEATURE_XNX, "xnx" },
		{ CPU_FEATURE_CCIDX, "ccidx" },
		{ CPU_FEATURE_CONSTPACFIELD, "constpacfield" },
		{ CPU_FEATURE_EPAC, "epac" },
		{ CPU_FEATURE_FCMA, "fcma" },
		{ CPU_FEATURE_FPAC, "fpac" },
		{ CPU_FEATURE_FPACCOMBINE, "fpaccombine" },
		{ CPU_FEATURE_JSCVT, "jscvt" },
		{ CPU_FEATURE_LRCPC, "lrcpc" },
		{ CPU_FEATURE_PACIMP, "pacimp" },
		{ CPU_FEATURE_PACQARMA3, "pacqarma3" },
		{ CPU_FEATURE_PACQARMA5, "pacqarma5" },
		{ CPU_FEATURE_PAUTH, "pauth" },
		{ CPU_FEATURE_SPEV1P1, "spev1p1" },
		{ CPU_FEATURE_AMUV1, "amuv1" },
		{ CPU_FEATURE_BBM, "bbm" },
		{ CPU_FEATURE_DIT, "dit" },
		{ CPU_FEATURE_DEBUGV8P4, "debugv8p4" },
		{ CPU_FEATURE_DOTPROD, "dotprod" },
		{ CPU_FEATURE_DOUBLEFAULT, "doublefault" },
		{ CPU_FEATURE_FHM, "fhm" },
		{ CPU_FEATURE_FLAGM, "flagm" },
		{ CPU_FEATURE_IDST, "idst" },
		{ CPU_FEATURE_LRCPC2, "lrcpc2" },
		{ CPU_FEATURE_LSE2, "lse2" },
		{ CPU_FEATURE_MPAM, "mpam" },
		{ CPU_FEATURE_PMUV3P4, "pmuv3p4" },
		{ CPU_FEATURE_RASV1P1, "rasv1p1" },
		{ CPU_FEATURE_S2FWB, "s2fwb" },
		{ CPU_FEATURE_SEL2, "sel2" },
		{ CPU_FEATURE_TLBIOS, "tlbios" },
		{ CPU_FEATURE_TLBIRANGE, "tlbirange" },
		{ CPU_FEATURE_TRF, "trf" },
		{ CPU_FEATURE_TTL, "ttl" },
		{ CPU_FEATURE_TTST, "ttst" },
		{ CPU_FEATURE_BTI, "bti" },
		{ CPU_FEATURE_CSV2, "csv2" },
		{ CPU_FEATURE_CSV3, "csv3" },
		{ CPU_FEATURE_DPB2, "dpb2" },
		{ CPU_FEATURE_E0PD, "e0pd" },
		{ CPU_FEATURE_EVT, "evt" },
		{ CPU_FEATURE_EXS, "exs" },
		{ CPU_FEATURE_FRINTTS, "frintts" },
		{ CPU_FEATURE_FLAGM2, "flagm2" },
		{ CPU_FEATURE_MTE, "mte" },
		{ CPU_FEATURE_MTE2, "mte2" },
		{ CPU_FEATURE_PMUV3P5, "pmuv3p5" },
		{ CPU_FEATURE_RNG, "rng" },
		{ CPU_FEATURE_RNG_TRAP, "rng_trap" },
		{ CPU_FEATURE_SB, "sb" },
		{ CPU_FEATURE_SPECRES, "specres" },
		{ CPU_FEATURE_SSBS, "ssbs" },
		{ CPU_FEATURE_SSBS2, "ssbs2" },
		{ CPU_FEATURE_AMUV1P1, "amuv1p1" },
		{ CPU_FEATURE_BF16, "bf16" },
		{ CPU_FEATURE_DGH, "dgh" },
		{ CPU_FEATURE_ECV, "ecv" },
		{ CPU_FEATURE_FGT, "fgt" },
		{ CPU_FEATURE_MPAMV0P1, "mpamv0p1" },
		{ CPU_FEATURE_MPAMV1P1, "mpamv1p1" },
		{ CPU_FEATURE_MTPMU, "mtpmu" },
		{ CPU_FEATURE_PAUTH2, "pauth2" },
		{ CPU_FEATURE_TWED, "twed" },
		{ CPU_FEATURE_AFP, "afp" },
		{ CPU_FEATURE_EBF16, "ebf16" },
		{ CPU_FEATURE_HCX, "hcx" },
		{ CPU_FEATURE_LPA2, "lpa2" },
		{ CPU_FEATURE_LS64, "ls64" },
		{ CPU_FEATURE_LS64_ACCDATA, "ls64_accdata" },
		{ CPU_FEATURE_LS64_V, "ls64_v" },
		{ CPU_FEATURE_MTE3, "mte3" },
		{ CPU_FEATURE_MTE_ASYM_FAULT, "mte_asym_fault" },
		{ CPU_FEATURE_PAN3, "pan3" },
		{ CPU_FEATURE_PMUV3P7, "pmuv3p7" },
		{ CPU_FEATURE_RPRES, "rpres" },
		{ CPU_FEATURE_SPEV1P2, "spev1p2" },
		{ CPU_FEATURE_WFXT, "wfxt" },
		{ CPU_FEATURE_XS, "xs" },
		{ CPU_FEATURE_CMOW, "cmow" },
		{ CPU_FEATURE_DEBUGV8P8, "debugv8p8" },
		{ CPU_FEATURE_HBC, "hbc" },
		{ CPU_FEATURE_MOPS, "mops" },
		{ CPU_FEATURE_NMI, "nmi" },
		{ CPU_FEATURE_PMUV3P8, "pmuv3p8" },
		{ CPU_FEATURE_SCTLR2, "sctlr2" },
		{ CPU_FEATURE_SPEV1P3, "spev1p3" },
		{ CPU_FEATURE_TCR2, "tcr2" },
		{ CPU_FEATURE_TIDCP1, "tidcp1" },
		{ CPU_FEATURE_ADERR, "aderr" },
		{ CPU_FEATURE_AIE, "aie" },
		{ CPU_FEATURE_ANERR, "anerr" },
		{ CPU_FEATURE_ATS1A, "ats1a" },
		{ CPU_FEATURE_CLRBHB, "clrbhb" },
		{ CPU_FEATURE_CSSC, "cssc" },
		{ CPU_FEATURE_DEBUGV8P9, "debugv8p9" },
		{ CPU_FEATURE_DOUBLEFAULT2, "doublefault2" },
		{ CPU_FEATURE_ECBHB, "ecbhb" },
		{ CPU_FEATURE_FGT2, "fgt2" },
		{ CPU_FEATURE_HAFT, "haft" },
		{ CPU_FEATURE_LRCPC3, "lrcpc3" },
		{ CPU_FEATURE_MTE4, "mte4" },
		{ CPU_FEATURE_MTE_ASYNC, "mte_async" },
		{ CPU_FEATURE_MTE_CANONICAL_TAGS, "mte_canonical_tags" },
		{ CPU_FEATURE_MTE_NO_ADDRESS_TAGS, "mte_no_address_tags" },
		{ CPU_FEATURE_MTE_PERM, "mte_perm" },
		{ CPU_FEATURE_MTE_STORE_ONLY, "mte_store_only" },
		{ CPU_FEATURE_MTE_TAGGED_FAR, "mte_tagged_far" },
		{ CPU_FEATURE_PFAR, "pfar" },
		{ CPU_FEATURE_PMUV3_ICNTR, "pmuv3_icntr" },
		{ CPU_FEATURE_PMUV3_SS, "pmuv3_ss" },
		{ CPU_FEATURE_PMUV3P9, "pmuv3p9" },
		{ CPU_FEATURE_PRFMSLC, "prfmslc" },
		{ CPU_FEATURE_RASV2, "rasv2" },
		{ CPU_FEATURE_RPRFM, "rprfm" },
		{ CPU_FEATURE_S1PIE, "s1pie" },
		{ CPU_FEATURE_S1POE, "s1poe" },
		{ CPU_FEATURE_S2PIE, "s2pie" },
		{ CPU_FEATURE_S2POE, "s2poe" },
		{ CPU_FEATURE_SPECRES2, "specres2" },
		{ CPU_FEATURE_SPE_DPFZS, "spe_dpfzs" },
		{ CPU_FEATURE_SPEV1P4, "spev1p4" },
		{ CPU_FEATURE_SPMU, "spmu" },
		{ CPU_FEATURE_THE, "the" },
		{ CPU_FEATURE_SVE2, "sve2" },
		{ CPU_FEATURE_SVE_AES, "sve_aes" },
		{ CPU_FEATURE_SVE_BITPERM, "sve_bitperm" },
		{ CPU_FEATURE_SVE_PMULL128, "sve_pmull128" },
		{ CPU_FEATURE_SVE_SHA3, "sve_sha3" },
		{ CPU_FEATURE_SVE_SM4, "sve_sm4" },
		{ CPU_FEATURE_TME, "tme" },
		{ CPU_FEATURE_TRBE, "trbe" },
		{ CPU_FEATURE_BRBE, "brbe" },
		{ CPU_FEATURE_RME, "rme" },
		{ CPU_FEATURE_SME, "sme" },
		{ CPU_FEATURE_SME_F64F64, "sme_f64f64" },
		{ CPU_FEATURE_SME_FA64, "sme_fa64" },
		{ CPU_FEATURE_SME_I16I64, "sme_i16i64" },
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
		if (_libcpuid_errno == matchtable[i].error)
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
			cpuid_set_error(ERR_NO_CPUID);
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
			cpuid_get_list_arm(vendor, list);
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
			cpuid_get_list_centaur(list);
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
		case VENDOR_ARM:
		case VENDOR_BROADCOM:
		case VENDOR_CAVIUM:
		case VENDOR_DEC:
		case VENDOR_FUJITSU:
		case VENDOR_HISILICON:
		case VENDOR_INFINEON:
		case VENDOR_FREESCALE:
		case VENDOR_NVIDIA:
		case VENDOR_APM:
		case VENDOR_QUALCOMM:
		case VENDOR_SAMSUNG:
		case VENDOR_MARVELL:
		case VENDOR_APPLE:
		case VENDOR_FARADAY:
		case VENDOR_MICROSOFT:
		case VENDOR_PHYTIUM:
		case VENDOR_AMPERE:
			cpuid_get_list_arm(vendor, list);
			break;
		default:
			warnf("Unknown vendor passed to cpuid_get_cpu_list()\n");
			cpuid_set_error(ERR_INVRANGE);
			list->num_entries = 0;
			list->names = NULL;
			break;
 	}
}

hypervisor_vendor_t cpuid_get_hypervisor(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int i, r;
	uint32_t hypervisor_fn40000000h[NUM_REGS];
	char hypervisor_str[VENDOR_STR_MAX];
	struct cpu_id_t mydata;
	const struct { hypervisor_vendor_t hypervisor; char match[16]; }
	matchtable[NUM_HYPERVISOR_VENDORS] = {
		/* source: https://github.com/a0rtega/pafish/blob/master/pafish/cpu.c */
		{ HYPERVISOR_BHYVE      , "bhyve bhyve\0"   },
		{ HYPERVISOR_HYPERV     , "Microsoft Hv"    },
		{ HYPERVISOR_KVM        , "KVMKVMKVM\0\0\0" },
		{ HYPERVISOR_PARALLELS  , "prl hyperv\0\0"  },
		{ HYPERVISOR_QEMU       , "TCGTCGTCGTCG"    },
		{ HYPERVISOR_VIRTUALBOX , "VBoxVBoxVBox"    },
		{ HYPERVISOR_VMWARE     , "VMwareVMware"    },
		{ HYPERVISOR_XEN        , "XenVMMXenVMM"    },
	};

	if (!data) {
		if ((r = cpu_identify(raw, &mydata)) < 0)
			return HYPERVISOR_UNKNOWN;
		data = &mydata;
	}

	/* Intel and AMD CPUs have reserved bit 31 of ECX of CPUID leaf 0x1 as the hypervisor present bit
	Source: https://kb.vmware.com/s/article/1009458 */
	switch (data->vendor) {
		case VENDOR_AMD:
		case VENDOR_INTEL:
			break;
		default:
			return HYPERVISOR_UNKNOWN;
	}
	if (!data->flags[CPU_FEATURE_HYPERVISOR])
		return HYPERVISOR_NONE;

	/* Intel and AMD have also reserved CPUID leaves 0x40000000 - 0x400000FF for software use.
	Hypervisors can use these leaves to provide an interface to pass information
	from the hypervisor to the guest operating system running inside a virtual machine.
	The hypervisor bit indicates the presence of a hypervisor
	and that it is safe to test these additional software leaves. */
	memset(hypervisor_fn40000000h, 0, sizeof(hypervisor_fn40000000h));
	hypervisor_fn40000000h[EAX] = 0x40000000;
	cpu_exec_cpuid_ext(hypervisor_fn40000000h);

	/* Copy the hypervisor CPUID information leaf */
	memcpy(hypervisor_str + 0, &hypervisor_fn40000000h[1], 4);
	memcpy(hypervisor_str + 4, &hypervisor_fn40000000h[2], 4);
	memcpy(hypervisor_str + 8, &hypervisor_fn40000000h[3], 4);
	hypervisor_str[12] = '\0';

	/* Determine hypervisor */
	for (i = 0; i < NUM_HYPERVISOR_VENDORS; i++)
		if (!strcmp(hypervisor_str, matchtable[i].match))
			return matchtable[i].hypervisor;
	return HYPERVISOR_UNKNOWN;
}

void cpuid_free_cpu_list(struct cpu_list_t* list)
{
	int i;
	if (list->num_entries <= 0) return;
	for (i = 0; i < list->num_entries; i++)
		free(list->names[i]);
	free(list->names);
	list->names = NULL;
	list->num_entries = 0;
}

void cpuid_free_raw_data_array(struct cpu_raw_data_array_t* raw_array)
{
	if (raw_array->num_raw <= 0) return;
	free(raw_array->raw);
	raw_array->num_raw = 0;
}

void cpuid_free_system_id(struct system_id_t* system)
{
	if (system->num_cpu_types <= 0) return;
	free(system->cpu_types);
	system->num_cpu_types = 0;
}
