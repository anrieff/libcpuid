/*
 * Copyright 2024  Veselin Georgiev,
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

/* This kernel module is inspired on cpuctl: https://github.com/freebsd/freebsd-src/blob/master/sys/dev/cpuctl/cpuctl.c */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/ioccom.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/sched.h>
#include <sys/smp.h>
#include <sys/pmckern.h>

#include "libcpuid_arm_driver.h"

#define	CPUID_VERSION 1

#ifdef LIBCPUID_DRIVER_DEBUG
# define	DPRINTF(format,...) printf(format, __VA_ARGS__);
#else
# define	DPRINTF(...)
#endif

#if defined(__arm__)
# define cpuid_read_sysreg(aarch32, aarch64, value) asm volatile("mrc " aarch32 : "=r" (value))
#elif defined(__aarch64__)
# define cpuid_read_sysreg(aarch32, aarch64, value) { uint64_t __val; asm volatile("mrs %0, " aarch64 : "=r" (__val)); value = __val; }
#else
# error This platform is not supported by this kernel module
#endif

static d_ioctl_t cpuid_ioctl;
static struct cdev **cpuid_devs;
static MALLOC_DEFINE(M_CPUID, "cpuid", "CPUID buffer");

/*
 * This function checks if specified cpu enabled or not.
 */
static int
cpu_enabled(int cpu)
{
	return (pmc_cpu_is_disabled(cpu) == 0);
}

/*
 * Check if the current thread is bound to a specific cpu.
 */
static int
cpu_sched_is_bound(struct thread *td)
{
	int ret;

	thread_lock(td);
	ret = sched_is_bound(td);
	thread_unlock(td);
	return (ret);
}

/*
 * Switch to target cpu to run.
 */
static void
set_cpu(int cpu, struct thread *td)
{
	KASSERT(cpu >= 0 && cpu <= mp_maxid && cpu_enabled(cpu), ("[cpuid,%d]: bad cpu number %d", __LINE__, cpu));
	thread_lock(td);
	sched_bind(td, cpu);
	thread_unlock(td);
	KASSERT(td->td_oncpu == cpu, ("[cpuid,%d]: cannot bind to target cpu %d on cpu %d", __LINE__, cpu, td->td_oncpu));
}

static void
restore_cpu(int oldcpu, int is_bound, struct thread *td)
{
	KASSERT(oldcpu >= 0 && oldcpu <= mp_maxid && cpu_enabled(oldcpu), ("[cpuid,%d]: bad cpu number %d", __LINE__, oldcpu));
	thread_lock(td);
	if (is_bound == 0)
		sched_unbind(td);
	else
		sched_bind(td, oldcpu);
	thread_unlock(td);
}

/*
 * Actually perform register read.
 */
static int
__read_reg_on_cpu(int cpu, struct read_reg_t *read_reg, u_long cmd, struct thread *td)
{
	int is_bound = 0;
	int oldcpu;

	KASSERT(cpu >= 0 && cpu <= mp_maxid, ("[cpuid,%d]: bad cpu number %d", __LINE__, cpu));
	oldcpu = td->td_oncpu;
	is_bound = cpu_sched_is_bound(td);
	set_cpu(cpu, td);

	DPRINTF("[cpuid,%d]: operating on request %d\n", __LINE__, read_reg->request);
	read_reg->value_64b = 0;
	switch(read_reg->request)
	{
		case REQ_MIDR:     cpuid_read_sysreg("p15, 0, %0, c0, c0, 0", AARCH64_REG_MIDR_EL1,   read_reg->value_64b); break;
		case REQ_MPIDR:    cpuid_read_sysreg("p15, 0, %0, c0, c0, 5", AARCH64_REG_MPIDR_EL1,  read_reg->value_64b); break;
		case REQ_REVIDR:   cpuid_read_sysreg("p15, 0, %0, c0, c0, 6", AARCH64_REG_REVIDR_EL1, read_reg->value_64b); break;
		case REQ_ID_AFR0:  cpuid_read_sysreg("p15, 0, %0, c0, c1, 3", AARCH64_REG_ID_AFR0,    read_reg->value_32b); break;
		case REQ_ID_DFR0:  cpuid_read_sysreg("p15, 0, %0, c0, c1, 2", AARCH64_REG_ID_DFR0,    read_reg->value_32b); break;
		case REQ_ID_DFR1:  cpuid_read_sysreg("p15, 0, %0, c0, c3, 5", AARCH64_REG_ID_DFR1,    read_reg->value_32b); break;
		case REQ_ID_ISAR0: cpuid_read_sysreg("p15, 0, %0, c0, c2, 0", AARCH64_REG_ID_ISAR0,   read_reg->value_32b); break;
		case REQ_ID_ISAR1: cpuid_read_sysreg("p15, 0, %0, c0, c2, 1", AARCH64_REG_ID_ISAR1,   read_reg->value_32b); break;
		case REQ_ID_ISAR2: cpuid_read_sysreg("p15, 0, %0, c0, c2, 2", AARCH64_REG_ID_ISAR2,   read_reg->value_32b); break;
		case REQ_ID_ISAR3: cpuid_read_sysreg("p15, 0, %0, c0, c2, 3", AARCH64_REG_ID_ISAR3,   read_reg->value_32b); break;
		case REQ_ID_ISAR4: cpuid_read_sysreg("p15, 0, %0, c0, c2, 4", AARCH64_REG_ID_ISAR4,   read_reg->value_32b); break;
		case REQ_ID_ISAR5: cpuid_read_sysreg("p15, 0, %0, c0, c2, 5", AARCH64_REG_ID_ISAR5,   read_reg->value_32b); break;
		case REQ_ID_ISAR6: cpuid_read_sysreg("p15, 0, %0, c0, c2, 7", AARCH64_REG_ID_ISAR6,   read_reg->value_32b); break;
		case REQ_ID_MMFR0: cpuid_read_sysreg("p15, 0, %0, c0, c1, 4", AARCH64_REG_ID_MMFR0,   read_reg->value_32b); break;
		case REQ_ID_MMFR1: cpuid_read_sysreg("p15, 0, %0, c0, c1, 5", AARCH64_REG_ID_MMFR1,   read_reg->value_32b); break;
		case REQ_ID_MMFR2: cpuid_read_sysreg("p15, 0, %0, c0, c1, 6", AARCH64_REG_ID_MMFR2,   read_reg->value_32b); break;
		case REQ_ID_MMFR3: cpuid_read_sysreg("p15, 0, %0, c0, c1, 7", AARCH64_REG_ID_MMFR3,   read_reg->value_32b); break;
		case REQ_ID_MMFR4: cpuid_read_sysreg("p15, 0, %0, c0, c2, 6", AARCH64_REG_ID_MMFR4,   read_reg->value_32b); break;
		case REQ_ID_MMFR5: cpuid_read_sysreg("p15, 0, %0, c0, c3, 6", AARCH64_REG_ID_MMFR5,   read_reg->value_32b); break;
		case REQ_ID_PFR0:  cpuid_read_sysreg("p15, 0, %0, c0, c1, 0", AARCH64_REG_ID_PFR0,    read_reg->value_32b); break;
		case REQ_ID_PFR1:  cpuid_read_sysreg("p15, 0, %0, c0, c1, 1", AARCH64_REG_ID_PFR1,    read_reg->value_32b); break;
		case REQ_ID_PFR2:  cpuid_read_sysreg("p15, 0, %0, c0, c3, 4", AARCH64_REG_ID_PFR2,    read_reg->value_32b); break;
#if defined(__aarch64__)
		case REQ_ID_AA64AFR0:  cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64AFR0_EL1,  read_reg->value_64b); break;
		case REQ_ID_AA64AFR1:  cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64AFR1_EL1,  read_reg->value_64b); break;
		case REQ_ID_AA64DFR0:  cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64DFR0_EL1,  read_reg->value_64b); break;
		case REQ_ID_AA64DFR1:  cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64DFR1_EL1,  read_reg->value_64b); break;
		case REQ_ID_AA64DFR2:  cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64DFR2_EL1,  read_reg->value_64b); break;
		case REQ_ID_AA64FPFR0: cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64FPFR0_EL1, read_reg->value_64b); break;
		case REQ_ID_AA64ISAR0: cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64ISAR0_EL1, read_reg->value_64b); break;
		case REQ_ID_AA64ISAR1: cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64ISAR1_EL1, read_reg->value_64b); break;
		case REQ_ID_AA64ISAR2: cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64ISAR2_EL1, read_reg->value_64b); break;
		case REQ_ID_AA64ISAR3: cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64ISAR3_EL1, read_reg->value_64b); break;
		case REQ_ID_AA64MMFR0: cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64MMFR0_EL1, read_reg->value_64b); break;
		case REQ_ID_AA64MMFR1: cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64MMFR1_EL1, read_reg->value_64b); break;
		case REQ_ID_AA64MMFR2: cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64MMFR2_EL1, read_reg->value_64b); break;
		case REQ_ID_AA64MMFR3: cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64MMFR3_EL1, read_reg->value_64b); break;
		case REQ_ID_AA64MMFR4: cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64MMFR4_EL1, read_reg->value_64b); break;
		case REQ_ID_AA64PFR0:  cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64PFR0_EL1,  read_reg->value_64b); break;
		case REQ_ID_AA64PFR1:  cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64PFR1_EL1,  read_reg->value_64b); break;
		case REQ_ID_AA64PFR2:  cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64PFR2_EL1,  read_reg->value_64b); break;
		case REQ_ID_AA64SMFR0: cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64SMFR0_EL1, read_reg->value_64b); break;
		case REQ_ID_AA64ZFR0:  cpuid_read_sysreg(NULL, AARCH64_REG_ID_AA64ZFR0_EL1,  read_reg->value_64b); break;
#endif /* __aarch64__ */
		default:
			read_reg->request = REQ_INVALID;
			printf("[cpuid,%d]: unknown operation requested: %d", __LINE__, read_reg->request);
			break;
	}
	DPRINTF("[cpuid,%d]: set value 0x%016lX for cpu number %d\n", __LINE__, read_reg->value_64b, cpu);

	restore_cpu(oldcpu, is_bound, td);
	return 0;
}

int
cpuid_ioctl(struct cdev *dev, u_long cmd, caddr_t data, int flags, struct thread *td)
{
	int cpu, ret;

	cpu = dev2unit(dev);
	if (cpu > mp_maxid || !cpu_enabled(cpu)) {
		DPRINTF("[cpuid,%d]: bad cpu number %d\n", __LINE__, cpu);
		return (ENXIO);
	}

	DPRINTF("[cpuid,%d]: received ioctl %lu for cpu number %d\n", __LINE__, cmd, cpu);
	switch (cmd) {
		case ARM_IOC_READ_REG:
			ret = __read_reg_on_cpu(cpu, (struct read_reg_t *)data, cmd, td);
			break;

		default:
			ret = EINVAL;
			break;
	}

	return (ret);
}

static struct cdevsw cpuid_cdevsw = {
	.d_name =       "cpuid",
	.d_version =    D_VERSION,
	.d_ioctl =      cpuid_ioctl,
};

static int
cpuid_modevent(module_t mod __unused, int type, void *data __unused)
{
	int cpu;

	switch(type) {
		case MOD_LOAD:
			DPRINTF("[cpuid,%d]: load module\n", __LINE__);
			cpuid_devs = malloc(sizeof(*cpuid_devs) * (mp_maxid + 1), M_CPUID, M_WAITOK | M_ZERO);
			CPU_FOREACH(cpu)
				if (cpu_enabled(cpu))
					cpuid_devs[cpu] = make_dev(&cpuid_cdevsw, cpu, UID_ROOT, GID_KMEM, 0640, "cpuid%d", cpu);
			break;

		case MOD_UNLOAD:
			DPRINTF("[cpuid,%d]: unload module\n", __LINE__);
			CPU_FOREACH(cpu) {
				if (cpuid_devs[cpu] != NULL)
					destroy_dev(cpuid_devs[cpu]);
			}
			free(cpuid_devs, M_CPUID);
			break;

		case MOD_SHUTDOWN:
			break;

		default:
			return (EOPNOTSUPP);
	}

	return (0);
}

DEV_MODULE(cpuid, cpuid_modevent, NULL);
MODULE_VERSION(cpuid, CPUID_VERSION);
