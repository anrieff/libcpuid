/* A simple tool to decode the "CPUID Revision".
Build with:
gcc decode_cpuid_revision.c -lcpuid -o decode_cpuid_revision
*/
#include <stdio.h>
#include <stdlib.h>
#include "../libcpuid/libcpuid.h"

#define ANSWER_TO_THE_ULTIMATE_QUESTION_OF_LIFE_THE_UNIVERSE_AND_EVERYTHING 42


int main(int argc, char** argv)
{
	struct cpu_raw_data_t raw;
	struct cpu_id_t data;

	if (argc < 2) {
		printf("%s: x86 CPUID Revision is required as argument.\n", argv[0]);
		return 1;
	}
	raw.basic_cpuid[0][EAX] = ANSWER_TO_THE_ULTIMATE_QUESTION_OF_LIFE_THE_UNIVERSE_AND_EVERYTHING;
	raw.basic_cpuid[1][EAX] = strtol(argv[1], NULL, 16);
	cpu_identify(&raw, &data);

	printf("%s decoded to:\n", argv[1]);
	printf("  family     : %1$d (%1$02Xh)\n", data.x86.family);
	printf("  model      : %1$d (%1$02Xh)\n", data.x86.model);
	printf("  stepping   : %1$d (%1$02Xh)\n", data.x86.stepping);
	printf("  ext_family : %1$d (%1$02Xh)\n", data.x86.ext_family);
	printf("  ext_model  : %1$d (%1$02Xh)\n", data.x86.ext_model);
}
