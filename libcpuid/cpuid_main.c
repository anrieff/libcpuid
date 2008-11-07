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

static int cpuid_basic_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	return ERR_OK;
}

static void initialize_defaults(struct cpu_raw_data_t* raw)
{
	memset(raw, 0, sizeof(struct cpu_raw_data_t));
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
		cpu_exec_cpuid(0x8000000 + i, data->ext_cpuid[i]);
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
	
	fprintf(f, "version = %s\n", VERSION);
	for (i = 0; i < MAX_CPUID_LEVEL; i++)
		fprintf(f, "basic_cpuid[%d] = %08x %08x %08x %08x", i,
			data->basic_cpuid[i][0], data->basic_cpuid[i][1],
			data->basic_cpuid[i][2], data->basic_cpuid[i][3]);
	for (i = 0; i < MAX_EXT_CPUID_LEVEL; i++)
		fprintf(f, "ext_cpuid[%d] = %08x %08x %08x %08x", i,
			data->ext_cpuid[i][0], data->ext_cpuid[i][1],
			data->ext_cpuid[i][2], data->ext_cpuid[i][3]);
	for (i = 0; i < MAX_INTELFN4_LEVEL; i++)
		fprintf(f, "intel_fn4[%d] = %08x %08x %08x %08x", i,
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
	
	initialize_defaults(data);
	
	f = fopen(filename, "rt");
	if (!f) return set_error(ERR_OPEN);
	while (fgets(line, sizeof(line), f)) {
		++cur_line;
		len = strlen(line);
		if (len < 2) continue;
		if (line[len - 1] == '\n')
			line[--len] = '\0';
		for (i = 0; i < len && line[i] != '='; i++)
		if (i >= len && i < 2 && len - i - 2 <= 0) {
			fclose(f);
			return set_error(ERR_BADFMT);
		}
		strncpy(token, line, i - 1);
		token[i - 1] = '\0';
		value = &line[i + 2];
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
