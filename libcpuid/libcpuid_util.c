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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "libcpuid.h"
#include "libcpuid_util.h"

void match_features(const struct feature_map_t* matchtable, int count, uint32_t reg, struct cpu_id_t* data)
{
	int i;
	for (i = 0; i < count; i++)
		if (reg & (1 << matchtable[i].bit))
			data->flags[matchtable[i].feature] = 1;
}

static void default_warn(const char *msg)
{
	fprintf(stderr, "%s", msg);
}

libcpuid_warn_fn_t _warn_fun = default_warn;

void warnf(const char* format, ...)
{
	char buff[1024];
	va_list va;
	if (!_warn_fun) return;
	va_start(va, format);
	vsnprintf(buff, sizeof(buff), format, va);
	va_end(va);
	_warn_fun(buff);
}

static int score(const struct match_entry_t* entry, int family, int model,
                 int stepping, int xfamily, int xmodel, int code)
{
	int res = 0;
	if (entry->family	== family  ) res += 2;
	if (entry->model	== model   ) res += 2;
	if (entry->stepping	== stepping) res += 2;
	if (entry->ext_family	== xfamily ) res += 2;
	if (entry->ext_model	== xmodel  ) res += 2;
	if (entry->code		== code    ) res += 2;
	return res;
}

void match_cpu_codename(const struct match_entry_t* matchtable, int count,
                        struct cpu_id_t* data, int code)
{
	int bestscore = -1;
	int bestindex = 0;
	int i, t;
	
	for (i = 0; i < count; i++) {
		t = score(&matchtable[i], data->family, data->model,
		              data->stepping, data->ext_model,
		              data->ext_family, code);
		if (t > bestscore) {
			bestscore = t;
			bestindex = i;
		}
	}
	strcpy(data->cpu_codename, matchtable[bestindex].name);
}
