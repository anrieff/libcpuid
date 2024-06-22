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
#ifndef __ASM_BITS_H__
#define __ASM_BITS_H__
#include "libcpuid.h"

/* Determine Compiler: */
#if defined(_MSC_VER)
#if !defined(COMPILER_MICROSOFT)
#	define COMPILER_MICROSOFT
#endif
#elif defined(__GNUC__)
#if !defined(COMPILER_GCC)
#	define COMPILER_GCC
#endif
#elif defined(__clang__)
#if !defined(COMPILER_CLANG)
#	define COMPILER_CLANG
#endif
#endif

/* Determine Platform */
#if defined(__x86_64__) || defined(_M_AMD64)
#if !defined(PLATFORM_X64)
#	define PLATFORM_X64
#endif
#elif defined(__i386__) || defined(_M_IX86)
#if !defined(PLATFORM_X86)
#	define PLATFORM_X86
#endif
#elif defined(__arm__)
#if !defined(PLATFORM_ARM)
#	define PLATFORM_ARM
#endif
#elif defined(__aarch64__)
#if !defined(PLATFORM_AARCH64)
#	define PLATFORM_AARCH64
/*
 * Older assemblers don't recognize newer system register names,
 * but we can still access them by the Sn_n_Cn_Cn_n syntax.
 */
#	define SYS_ID_AA64PFR0_EL1  "S3_0_C0_C4_0"
#	define SYS_ID_AA64PFR1_EL1  "S3_0_C0_C4_1"
#	define SYS_ID_AA64PFR2_EL1  "S3_0_C0_C4_2"
#	define SYS_ID_AA64ZFR0_EL1  "S3_0_C0_C4_4"
#	define SYS_ID_AA64SMFR0_EL1 "S3_0_C0_C4_5"
#	define SYS_ID_AA64DFR0_EL1  "S3_0_C0_C5_0"
#	define SYS_ID_AA64DFR1_EL1  "S3_0_C0_C5_1"
#	define SYS_ID_AA64ISAR0_EL1 "S3_0_C0_C6_0"
#	define SYS_ID_AA64ISAR1_EL1 "S3_0_C0_C6_1"
#	define SYS_ID_AA64ISAR2_EL1 "S3_0_C0_C6_2"
#	define SYS_ID_AA64MMFR0_EL1 "S3_0_C0_C7_0"
#	define SYS_ID_AA64MMFR1_EL1 "S3_0_C0_C7_1"
#	define SYS_ID_AA64MMFR2_EL1 "S3_0_C0_C7_2"
#	define SYS_ID_AA64MMFR3_EL1 "S3_0_C0_C7_3"
#	define SYS_ID_AA64MMFR4_EL1 "S3_0_C0_C7_4"
#endif
#endif

/* Under Windows/AMD64 with MSVC, inline assembly isn't supported */
#if (defined(COMPILER_GCC) || defined(COMPILER_CLANG)) || \
	(defined(COMPILER_MICROSOFT) && defined(PLATFORM_X86))
#	define INLINE_ASM_SUPPORTED
#endif

#ifdef INLINE_ASM_SUPPORTED
#  if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#    define cpu_exec_mrs(reg_name, reg_value) __asm __volatile("mrs %0, " reg_name : "=r" (reg_value))
#  elif defined(COMPILER_MICROSOFT)
#    define cpu_exec_mrs(reg_name, reg_value) __asm { mrs reg_value, reg_name }
#  else
#    error "Unsupported compiler"
#  endif /* COMPILER */
#endif /* INLINE_ASM_SUPPORTED */

int cpuid_exists_by_eflags(void);
void exec_cpuid(uint32_t *regs);
void busy_sse_loop(int cycles);

#endif /* __ASM_BITS_H__ */
