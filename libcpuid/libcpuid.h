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
/**
 * @File     libcpuid.h
 * @Author   Veselin Georgiev
 * @Date     Oct 2008
 * @Version  0.1.0
 *
 * Version history:
 *
 *  0.1.0 (2008-10-15): initial adaptation from wxfractgui sources
 */
/** @defgroup libcpuid
 *  @{
 */

#ifndef __LIBCPUID_H__
#define __LIBCPUID_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Include some integer type specifications: */
#include "libcpuid_types.h"

/* Some limits and other constants */
#include "libcpuid_constants.h"

/**
 * @brief CPU vendor, as guessed from the Vendor String.
 */
enum _cpu_vendor_t {
	INTEL = 0,
	AMD,
	CYRIX,
	VIA,
	NEXTGEN,
	TRANSMETA,
	UMC,
	CENTAUR,
	RISE,
	SIS,
	NSC,
	
	NUM_VENDORS,
	UNKNOWN = -1,
};
typedef enum _cpu_vendor_t cpu_vendor_t;

/**
 * @brief Contains just the raw CPUID data.
 *
 * This contains only the most basic CPU data, required to do identification
 * and feature recognition. Every processor should be identifiable using this
 * data only.
 */
struct cpu_raw_data_t {
	/** contains results of CPUID for eax = 0, 1, ...*/
	uint32_t basic_cpuid[32][4];

	/** contains results of CPUID for eax = 0x80000000, 0x80000001, ...*/
	uint32_t ext_cpuid[32][4];
};

/**
 * @brief This contains the recognized CPU features/info
 */
struct cpu_id_t {
	/** contains the vendor string, as in cpu_raw_data_t */
	char vendor_str[VENDOR_STR_MAX];
	
	/** contains the brand string, as in cpu_raw_data_t */
	char brand_str[BRAND_STR_MAX];
	
	/** contains the recognized CPU vendor, as in cpu_raw_data_t */
	cpu_vendor_t vendor;
	
	/**
	 * contain CPU flags. Used to test for features. See 
	 * the CPU_FEATURE_* macros below.
	 */
	uint8_t flags[CPU_FLAGS_MAX];
	
	/** CPU family */
	int32_t family;
	
	/** CPU model */
	int32_t model;
	
	/** CPU stepping */
	int32_t stepping;
	
	/** CPU extended family (mainly on Intel CPUs) */
	int32_t ext_family;
	
	/** CPU extended model (mainly on Intel CPUs) */
	int32_t ext_model;
	
	/** Number of CPU cores on the current processor */
	int32_t num_cores;
	
	/**
	 * Number of logical processors on the current processor.
	 * Could be more than the actual number of physical cores, e.g.
	 * if the processor has Hyperthreading.
	 */
	int32_t num_logical_cpus;
	
	/**
	 * The total number of logical processors.
	 *
	 * This is num_logical_cpus * {total physical processors in the system}
	 *
	 * If you're writing a multithreaded program and you want to run it on
	 * all CPUs, this is the number of threads you need.
	 */
	int32_t total_cpus;
	
	/**
	 * L1 data cache size in KB. Could be zero, if the CPU lacks cache.
	 * If the size cannot be determined, it will be -1.
	 */
	int32_t l1_data_cache;
	
	/**
	 * L1 instruction cache size in KB. Could be zero, if the CPU lacks
	 * cache. If the size cannot be determined, it will be -1.
	 * @note On some Intel CPUs, whose instruction cache is in fact
	 * a trace cache, the size will be expressed in K uOps 
	 */
	int32_t l1_instruction_cache;
	
	/**
	 * L2 cache size in KB. Could be zero, if the CPU lacks L2 cache.
	 * If the size of the cache could not be determined, it will be -1
	 */
	int32_t l2_cache;
	
	/** L3 cache size in KB. Zero on most systems */
	int32_t l3_cache;
	
	/** Cache associativity for the L1 data cache. -1 if undetermined */
	int32_t l1_assoc;
	
	/** Cache associativity for the L2 cache. -1 if undetermined */
	int32_t l2_assoc;
	
	/** Cache associativity for the L3 cache. -1 if undetermined */
	int32_t l3_assoc;
	
	/**
	 * The brief and human-friendly CPU codename, which was recognized.<br>
	 * Examples:
	 * @code
	 * +--------+--------+-------+-------+-------+---------------------------------------+-----------------------+
	 * | Vendor | Family | Model | Step. | Cache |       Brand String                    | cpu_id_t.cpu_codename |
	 * +--------+--------+-------+-------+-------+---------------------------------------+-----------------------+
	 * | AMD    |      6 |     8 |     0 |   256 | (not available - will be ignored)     | "K6-2"                |
	 * | Intel  |     15 |     2 |     5 |   512 | "Intel(R) Xeon(TM) CPU 2.40GHz"       | "Xeon (Prestonia)"    |
	 * | Intel  |      6 |    15 |    11 |  4096 | "Intel(R) Core(TM)2 Duo CPU E6550..." | "Conroe (Core 2 Duo)" |
	 * | AMD    |     15 |    35 |     2 |  1024 | "Dual Core AMD Opteron(tm) Proces..." | "Opteron (Dual Core)" |
	 * +--------+--------+-------+-------+-------+---------------------------------------+-----------------------+
	 * @endcode
	 */
	char cpu_codename[64];
};

/**
 * @brief CPU feature identifiers
 *
 * Usage:
 * @code
 * ...
 * cpu_id_t id;
 * ...
 * if (id.flags[CPU_FEATURE_SSE2]) {
 *     // The CPU has SSE2...
 *     ...
 * }
 * @endcode
 */
enum _cpu_feature_t {
	CPU_FEATURE_FPU = 0,	/*!< Floating point unit */
	CPU_FEATURE_VME,	/*!< Virtual mode extension */
	CPU_FEATURE_DEBUG,	/*!< Debugging extension */
	CPU_FEATURE_PSE,	/*!< Page size extension */
	CPU_FEATURE_TSC,	/*!< Time-stamp counter */
	CPU_FEATURE_MSR,	/*!< Model-specific regsisters, RDMSR/WRMSR supported */
	CPU_FEATURE_PAE,	/*!< Physical address extension */
	CPU_FEATURE_MCE,	/*!< Machine check exception */
	CPU_FEATURE_CX8,	/*!< CMPXCHG8B instruction supported */
	CPU_FEATURE_APIC,	/*!< APIC support */
	CPU_FEATURE_SEP,	/*!< SYSCALL / SYSEXIT supported */
	CPU_FEATURE_MTRR,	/*!< Memory type range registers */
	CPU_FEATURE_PGE,	/*!< Page global enable */
	CPU_FEATURE_MCA,	/*!< Machine check architecture */
	CPU_FEATURE_CMOV,	/*!< CMOVxx instructions supported */
	CPU_FEATURE_PAT,	/*!< Page attribute table */
	CPU_FEATURE_PSE36,	/*!< 36-bit page address extension */
	CPU_FEATURE_PSN,	/*!< Processor serial # implemented (Intel P3 only) */
	CPU_FEATURE_CLFLUSH,	/*!< CLFLUSH instruction supported */
	CPU_FEATURE_DTS,	/*!< Debug store supported */
	CPU_FEATURE_ACPI,	/*!< ACPI support (power states) */
	CPU_FEATURE_MMX,	/*!< MMX instruction set supported */
	CPU_FEATURE_FXSR,	/*!< FXSAVE / FXRSTOR supported */
	CPU_FEATURE_SSE,	/*!< Streaming-SIMD Extensions (SSE) supported */
	CPU_FEATURE_SSE2,	/*!< SSE2 instructions supported */
	CPU_FEATURE_SS,		/*!< Self-snoop */
	CPU_FEATURE_HT,		/*!< Hyper-threading supported (but might be disabled) */
	CPU_FEATURE_TM,		/*!< Thermal monitor */
	CPU_FEATURE_IA64,	/*!< IA64 supported (Itanium only) */
	CPU_FEATURE_PBE,	/*!< Pending-break enable */
	CPU_FEATURE_SSE3,	/*!< SSE3 instructions supported */
	CPU_FEATURE_PCLMUL,	/*!< PCLMULQDQ instruction supported */
	CPU_FEATURE_DTS64,	/*!< 64-bit Debug store supported */
	CPU_FEATURE_MON,	/*!< MONITOR / MWAIT supported */
	CPU_FEATURE_DSCPL,	/*!< CPL Qualified Debug Store */
	CPU_FEATURE_VMX,	/*!< Virtualization technology supported */
	CPU_FEATURE_SMX,	/*!< Safer mode exceptions */
	CPU_FEATURE_EST,	/*!< Enhanced SpeedStep */
	CPU_FEATURE_TM2,	/*!< Thermal monitor 2 */
	CPU_FEATURE_SSSE3,	/*!< SSSE3 instructionss supported (this is different from SSE3!) */
	CPU_FEATURE_CID,	/*!< Context ID supported */
	CPU_FEATURE_CX16,	/*!< CMPXCHG16B instruction supported */
	CPU_FEATURE_ETPRD,	/*!< Send Task Priority Messages disable */
	CPU_FEATURE_PDCM,	/*!< Performance capabilities MSR supported */
	CPU_FEATURE_DCA,	/*!< Direct cache access supported */
	CPU_FEATURE_SSE41,	/*!< SSE 4.1 instructions supported */
	CPU_FEATURE_SSE42,	/*!< SSE 4.2 instructions supported */
	CPU_FEATURE_MOVBE,	/*!< MOVBE instruction supported */
	CPU_FEATURE_POPCNT,	/*!< POPCNT instruction supported */
	CPU_FEATURE_AES,	/*!< AES* instructions supported */
	CPU_FEATURE_XSAVE,	/*!< XSAVE/XRSTOR/etc instructions supported */
	CPU_FEATURE_OSXSAVE,	/*!< non-privileged copy of OSXSAVE supported */
	CPU_FEATURE_AVX,	/*!< Advanced vector extensions supported */
	// AMD specific:
	CPU_FEATURE_MMXEXT,	/*!< AMD MMX-extended instructions supported */
	CPU_FEATURE_3DNOW,	/*!< AMD 3DNow! instructions supported */
	CPU_FEATURE_3DNOWEXT,	/*!< AMD 3DNow! extended instructions supported */
	CPU_FEATURE_NX,		/*!< No-execute bit supported */
	CPU_FEATURE_RDTSCP,	/*!< RDTSCP instruction supported (AMD-only) */
	CPU_FEATURE_LM,		/*!< Long mode supported */
	CPU_FEATURE_LAHFSAHF,	/*!< LAHF/SAHF supported in 64-bit mode */
	CPU_FEATURE_SVM,	/*!< AMD Secure virtual machine */
	CPU_FEATURE_LZCNT,	/*!< LZCNT instruction support */
	CPU_FEATURE_3DNOWPREFETCH,	/*!< PREFETCH/PREFETCHW support */
	CPU_FEATURE_OSVW,	/*!< OS Visible Workaround (AMD) */
	CPU_FEATURE_IBS,	/*!< Instruction-based sampling */
	CPU_FEATURE_SSE51,	/*!< SSE 5.1 instructions supported */
	CPU_FEATURE_SKINIT,	/*!< SKINIT / STGI supported */
	CPU_FEATURE_WDT,	/*!< Watchdog timer support */
};
typedef enum _cpu_feature_t cpu_feature_t;

/**
 * @brief Describes common library error codes
 */
enum _cpuid_error_t {
	ERR_OK       =  0,	/*!< "No error" */
	ERR_NO_CPUID = -1,	/*!< "CPUID instruction is not supported" */
	ERR_NO_RDTSC = -2,	/*!< "RDTSC instruction is not supported" */
	ERR_NO_MEM   = -3,	/*!< "Memory allocation failed" */
	ERR_OPEN     = -4,	/*!< "File open operation failed" */
	ERR_BADFMT   = -5,	/*!< "Bad file format" */
	ERR_NOT_IMP  = -6, 	/*!< "Not implemented" */
};
typedef enum _cpuid_error_t cpuid_error_t;

/**
 * @brief Internal structure, used in cpu_tsc_mark, cpu_tsc_unmark and
 *        cpu_clock_by_mark
 */
struct cpu_mark_t {
	uint64_t tsc;
	uint64_t sys_clock;
};

int cpuid_present(void);

void cpu_exec_cpuid(uint32_t eax, uint32_t* regs);

void cpu_exec_cpuid_ext(uint32_t* regs);

int cpuid_get_raw_data(struct cpu_raw_data_t* data);

int cpuid_serialize_raw_data(struct cpu_raw_data_t* data, const char* filename);

int cpuid_deserialize_raw_data(struct cpu_raw_data_t* data, const char* filename);

int cpu_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data);

const char* cpu_feature_str(cpu_feature_t feature);

const char* cpuid_error(void);

void cpu_rdtsc(uint64_t* result);

void cpu_tsc_mark(struct cpu_mark_t* mark);

void cpu_tsc_unmark(struct cpu_mark_t* mark);

int cpu_clock_by_mark(struct cpu_mark_t* mark);

int cpu_clock_by_os(void);

int cpu_clock_measure(int millis, int triple_check);

int cpu_clock(void);

const char* cpuid_lib_version(void);

#ifdef __cplusplus
}; // extern "C"
#endif


/** @} */

#endif // __LIBCPUID_H__
