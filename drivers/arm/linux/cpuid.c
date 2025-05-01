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

/* This kernel module is inpired on msr: https://github.com/torvalds/linux/blob/master/arch/x86/kernel/msr.c */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/smp.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cpu.h>
#include <linux/version.h>

#include "libcpuid_arm_driver.h"


#ifdef LIBCPUID_DRIVER_DEBUG
# define	DPRINTF(format, ...) pr_info(format, __VA_ARGS__)
#else
# define	DPRINTF(...)
#endif

#if defined(__arm__)
# define cpuid_read_sysreg(aarch32, aarch64, value) asm volatile("mrc " aarch32 : "=r" (value))
#elif defined(__aarch64__)
# define cpuid_read_sysreg(aarch32, aarch64, value) asm volatile("mrs %0, " aarch64 : "=r" (value))
#else
# error This platform is not supported by this kernel module
#endif

static enum cpuhp_state cpuhp_cpuid_state;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0))
static char *cpuid_devnode(struct device *dev, umode_t *mode)
#else
static char *cpuid_devnode(const struct device *dev, umode_t *mode)
#endif /* Linux 6.2 */
{
	return kasprintf(GFP_KERNEL, "cpu/%u/cpuid", MINOR(dev->devt));
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
static struct class *cpuid_class;
#else
static const struct class cpuid_class = {
	.name		= "cpuid",
	.devnode	= cpuid_devnode,
};
#endif /* Linux 6.6 */

static void __read_reg_on_cpu(void *info)
{
	struct read_reg_t *read_reg = info;

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
			pr_warn("[cpuid,%d]: unknown operation requested: %d", __LINE__, read_reg->request);
			break;
	}

	DPRINTF("[cpuid,%d]: set value 0x%016llX\n", __LINE__, read_reg->value_64b);
}

static long cpuid_ioctl(struct file *file, unsigned int ioc, unsigned long arg)
{
	u32 __user *uregs = (u32 __user *)arg;
	int err = 0;
	struct read_reg_t read_reg;
	const int cpu = iminor(file_inode(file));

	DPRINTF("[cpuid,%d]: received ioctl %u for cpu number %d\n", __LINE__, ioc, cpu);
	switch (ioc) {
		case ARM_IOC_READ_REG:
			if (!(file->f_mode & FMODE_READ)) {
				err = -EBADF;
				break;
			}
			if (copy_from_user(&read_reg, uregs, sizeof(read_reg))) {
				err = -EFAULT;
				break;
			}
			err = smp_call_function_single(cpu, __read_reg_on_cpu, &read_reg, 1);
			if (err)
				break;
			if (copy_to_user(uregs, &read_reg, sizeof(read_reg)))
				err = -EFAULT;
			break;

		default:
			err = -ENOTTY;
			break;
	}

	return err;
}

/*
 * File operations we support
 */
static const struct file_operations cpuid_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = cpuid_ioctl,
	.compat_ioctl = cpuid_ioctl,
};

static int cpuid_device_create(unsigned int cpu)
{
	struct device *dev;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
	dev = device_create(cpuid_class, NULL, MKDEV(CPUID_MAJOR, cpu), NULL, "cpuid%d", cpu);
#else
	dev = device_create(&cpuid_class, NULL, MKDEV(CPUID_MAJOR, cpu), NULL, "cpuid%d", cpu);
#endif /* Linux 6.6 */

	return PTR_ERR_OR_ZERO(dev);
}

static int cpuid_device_destroy(unsigned int cpu)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
	device_destroy(cpuid_class, MKDEV(CPUID_MAJOR, cpu));
#else
	device_destroy(&cpuid_class, MKDEV(CPUID_MAJOR, cpu));
#endif
	return 0;
}

static int __init cpuid_init(void)
{
	int err;

	DPRINTF("[cpuid,%d]: load module\n", __LINE__);
	if (__register_chrdev(CPUID_MAJOR, 0, NR_CPUS, "cpu/cpuid", &cpuid_fops)) {
		pr_err("unable to get major %d for cpuid\n", CPUID_MAJOR);
		return -EBUSY;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
# if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0))
	cpuid_class = class_create(THIS_MODULE, "cpuid");
# else
	cpuid_class = class_create("cpuid");
# endif /* Linux 6.4 */
	if (IS_ERR(cpuid_class)) {
		err = PTR_ERR(cpuid_class);
		goto out_chrdev;
	}
	cpuid_class->devnode = cpuid_devnode;
#else
	err = class_register(&cpuid_class);
	if (err)
		goto out_chrdev;
#endif /* Linux 6.6 */

#if defined(__aarch64__)
	err  = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "arm64/cpuid:online", cpuid_device_create, cpuid_device_destroy);
#else
	err  = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "arm/cpuid:online", cpuid_device_create, cpuid_device_destroy);
#endif
	if (err < 0)
		goto out_class;
	cpuhp_cpuid_state = err;
	return 0;

out_class:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
	class_destroy(cpuid_class);
#else
	class_unregister(&cpuid_class);
#endif /* Linux 6.6 */
out_chrdev:
	__unregister_chrdev(CPUID_MAJOR, 0, NR_CPUS, "cpu/cpuid");
	return err;
}
module_init(cpuid_init);

static void __exit cpuid_exit(void)
{
	DPRINTF("[cpuid,%d]: unload module\n", __LINE__);
	cpuhp_remove_state(cpuhp_cpuid_state);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
	class_destroy(cpuid_class);
#else
	class_unregister(&cpuid_class);
#endif /* Linux 6.6 */
	__unregister_chrdev(CPUID_MAJOR, 0, NR_CPUS, "cpu/cpuid");
}
module_exit(cpuid_exit)

MODULE_AUTHOR("The Tumultuous Unicorn Of Darkness");
MODULE_DESCRIPTION("ARM registers driver for libcpuid");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.1");
