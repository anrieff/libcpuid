/* This program can be used to convert CPUID Dump from http://instlatx64.atw.hu/
 * To build: gcc convert_instlatx64.c -o convert_instlatx64
 * To run: ./convert_instlatx64 <input file (from instlatx64)> <output file (without extension)>
 * Then, you can use the create_test.py script to create the test file */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../libcpuid/libcpuid.h"

#define LINE_LEN     60
#define FILENAME_LEN 128
#define CMD_LEN      256
#define EXT_CPUID    0x80000000

#define NOT_AFFECTED(__line)        (__line[0] == 0xffffffff) && (__line[1] == 0xffffffff) && (__line[2] == 0xffffffff) && (__line[3] == 0xffffffff)
#define AFFECT_DATA_LINE(__line)     __line[0] = eax ; __line[1] = ebx ; __line[2] = ecx ; __line[3] = edx
#define AFFECT_DATA_SUBLEAF(__array) __array[subleaf][0] = eax ; __array[subleaf][1] = ebx ; __array[subleaf][2] = ecx ; __array[subleaf][3] = edx

int serialize_raw_data(struct cpu_raw_data_t* data, FILE *f)
{
	int i;

	for (i = 0; i < MAX_CPUID_LEVEL; i++)
		fprintf(f, "basic_cpuid[%d]=%08x %08x %08x %08x\n", i,
			data->basic_cpuid[i][EAX], data->basic_cpuid[i][EBX],
			data->basic_cpuid[i][ECX], data->basic_cpuid[i][EDX]);
	for (i = 0; i < MAX_EXT_CPUID_LEVEL; i++)
		fprintf(f, "ext_cpuid[%d]=%08x %08x %08x %08x\n", i,
			data->ext_cpuid[i][EAX], data->ext_cpuid[i][EBX],
			data->ext_cpuid[i][ECX], data->ext_cpuid[i][EDX]);
	for (i = 0; i < MAX_INTELFN4_LEVEL; i++)
		fprintf(f, "intel_fn4[%d]=%08x %08x %08x %08x\n", i,
			data->intel_fn4[i][EAX], data->intel_fn4[i][EBX],
			data->intel_fn4[i][ECX], data->intel_fn4[i][EDX]);
	for (i = 0; i < MAX_INTELFN11_LEVEL; i++)
		fprintf(f, "intel_fn11[%d]=%08x %08x %08x %08x\n", i,
			data->intel_fn11[i][EAX], data->intel_fn11[i][EBX],
			data->intel_fn11[i][ECX], data->intel_fn11[i][EDX]);
	for (i = 0; i < MAX_INTELFN12H_LEVEL; i++)
		fprintf(f, "intel_fn12h[%d]=%08x %08x %08x %08x\n", i,
			data->intel_fn12h[i][EAX], data->intel_fn12h[i][EBX],
			data->intel_fn12h[i][ECX], data->intel_fn12h[i][EDX]);
	for (i = 0; i < MAX_INTELFN14H_LEVEL; i++)
		fprintf(f, "intel_fn14h[%d]=%08x %08x %08x %08x\n", i,
			data->intel_fn14h[i][EAX], data->intel_fn14h[i][EBX],
			data->intel_fn14h[i][ECX], data->intel_fn14h[i][EDX]);
	for (i = 0; i < MAX_AMDFN8000001DH_LEVEL; i++)
		fprintf(f, "amd_fn8000001dh[%d]=%08x %08x %08x %08x\n", i,
			data->amd_fn8000001dh[i][EAX], data->amd_fn8000001dh[i][EBX],
			data->amd_fn8000001dh[i][ECX], data->amd_fn8000001dh[i][EDX]);

	return 0;
}

int main(int argc, char *argv[])
{
	int assigned, subleaf;
	uint32_t addr, prev_addr, eax, ebx, ecx, edx;
	char line[LINE_LEN], raw_filename[FILENAME_LEN], report_filename[FILENAME_LEN], cmd[CMD_LEN];
	FILE *fin = NULL, *fout = NULL;
	struct cpu_raw_data_t *raw = &(struct cpu_raw_data_t) {};

	if(argc < 3)
	{
		fprintf(stderr, "Usage: %s <input file (from instlatx64)> <output file (without extension)>\n", argv[0]);
		return 1;
	}

	const char *input_filename  = argv[1];
	const char *output_filename = argv[2];
	snprintf(raw_filename,    FILENAME_LEN, "%s.raw",        output_filename);
	snprintf(report_filename, FILENAME_LEN, "%s_report.txt", output_filename);
	memset(raw, 0xff, sizeof(struct cpu_raw_data_t)); // ffffffff ffffffff ffffffff ffffffff means data is missing in output test file

	/* Open files */
	if((fin  = fopen(input_filename, "r")) == NULL)
	{
		perror("Failed to open input file");
		return 1;
	}
	if((fout = fopen(raw_filename, "w")) == NULL)
	{
		perror("Failed to open output file");
		return 1;
	}

	/* Parse file and store data in cpu_raw_data_t */
	prev_addr = -1;
	while(fgets(line, LINE_LEN, fin) != NULL)
	{
		int assigned = sscanf(line, "CPUID %x: %x-%x-%x-%x [SL %02i]", &addr, &eax, &ebx, &ecx, &edx, &subleaf);
		printf("[%i] %s", assigned, line);
		if(assigned >= 5)
		{
			if(addr < MAX_CPUID_LEVEL)
			{
				if(NOT_AFFECTED(raw->basic_cpuid[addr]))
				{
					AFFECT_DATA_LINE(raw->basic_cpuid[addr]);
				}
			}
			else if((addr >= EXT_CPUID) && ((addr - EXT_CPUID) < MAX_EXT_CPUID_LEVEL))
			{
				if(NOT_AFFECTED(raw->ext_cpuid[addr - EXT_CPUID]))
				{
					AFFECT_DATA_LINE(raw->ext_cpuid[addr - EXT_CPUID]);
				}
			}
		}
		if(assigned == 6)
		{
			switch (addr)
			{
				case 4:          AFFECT_DATA_SUBLEAF(raw->intel_fn4);       break;
				case 11:         AFFECT_DATA_SUBLEAF(raw->intel_fn11);      break;
				case 0x12:       AFFECT_DATA_SUBLEAF(raw->intel_fn12h);     break;
				case 0x14:       AFFECT_DATA_SUBLEAF(raw->intel_fn14h);     break;
				case 0x8000001D: AFFECT_DATA_SUBLEAF(raw->amd_fn8000001dh); break;
				default: break;
			}
		}
	}

	/* Write stored data in output file */
	serialize_raw_data(raw, fout);
	fclose(fout);

	/* Invoke cpuid_tool */
	snprintf(cmd, CMD_LEN, "../cpuid_tool/cpuid_tool --load=%s --report --outfile=%s", raw_filename, report_filename);
	system(cmd);

	/* Invoke create_test */
	snprintf(cmd, CMD_LEN, "./create_test.py %s %s > %s.test", raw_filename, report_filename, output_filename);
	if((argc > 3) && !strcmp(argv[3], "--create"))
	{
		system(cmd);
		remove(raw_filename);
		remove(report_filename);
	}
	else
	{
		printf("Done. Use the following command to create the test file:\n");
		printf("%s\n", cmd);
	}

	return 0;
}
