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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Implementation: */

static int _libcpiud_errno = ERR_OK;

static int cpuid_exists_by_eeflags(void)
{
#ifdef __x86_64__
	return 1; // CPUID is always present on the x86_64
#else
	
#endif
}

static void exec_cpiud(uint32_t *regs)
{
}

static int set_error(cpuid_error_t err)
{
	_libcpiud_errno = (int) err;
	return (int) err;
}

static int cpuid_basic_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	return ERR_OK;
}

/* Interface: */

int cpuid_present(void)
{
	return cpuid_exists_by_eeflags();
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
		cpu_exec_cpuid(0x8000000 + i, data->ext_cpuid[i]);
	return set_error(ERR_OK);
}

int cpuid_serialize_raw_data(struct cpu_raw_data_t* data, const char* filename)
{
	return set_error(ERR_NOT_IMP);
}

int cpuid_deserialize_raw_data(struct cpu_raw_data_t* data, const char* filename)
{
	return set_error(ERR_NOT_IMP);
}

int cpu_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int r;
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
	return ""; // FIXME
}

const char* cpuid_error(void)
{
	return ""; // FIXME
}


const char* cpuid_lib_version(void)
{
	return VERSION;
}
