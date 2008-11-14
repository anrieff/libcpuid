#include <stdio.h>
#include "libcpuid.h"

int main(void)
{
	if (!cpuid_present()) {
		printf("No CPUID on this CPU, sorry!\n");
		return -1;
	}
	printf("CPUID is present\n");
	struct cpu_raw_data_t raw;
	struct cpu_id_t data;
	if (cpuid_get_raw_data(&raw) < 0)
		printf("Error obtaining raw data: %s\n", cpuid_error());
	if (cpu_identify(&raw, &data) < 0)
		printf("Error identifying the CPU: %s\n", cpuid_error());
	printf("CPU Info:\n------------------\n");
	printf("  vendor_str : `%s'\n", data.vendor_str);
	printf("  vendor id  : %d\n", (int) data.vendor);
	printf("  brand_str  : `%s'\n", data.brand_str);
	printf("  family     : %d\n", data.family);
	printf("  model      : %d\n", data.model);
	printf("  stepping   : %d\n", data.stepping);
	printf("  ext_family : %d\n", data.ext_family);
	printf("  ext_model  : %d\n", data.ext_model);
	printf("  num_cores  : %d\n", data.num_cores);
	printf("  num_logical: %d\n", data.num_logical_cpus);
	printf("  total_cpus : %d\n", data.total_cpus);
	printf("  L1 D cache : %d KB\n", data.l1_data_cache);
	printf("  L1 I cache : %d KB\n", data.l1_instruction_cache);
	printf("  L2 cache   : %d KB\n", data.l2_cache);
	printf("  L3 cache   : %d KB\n", data.l3_cache);
	printf("  L1D assoc. : %d-way\n", data.l1_assoc);
	printf("  L2 assoc.  : %d-way\n", data.l2_assoc);
	printf("  L3 assoc.  : %d-way\n", data.l3_assoc);
	printf("  L1D line sz: %d-way\n", data.l1_cacheline);
	printf("  L2 line sz : %d-way\n", data.l2_cacheline);
	printf("  L3 line sz : %d-way\n", data.l3_cacheline);
	printf("  code name  : `%s'\n", data.cpu_codename);
	printf("  features   :");
	int i;
	for (i = 0; i < CPU_NUM_FEATURES; i++)
		if (data.flags[i])
			printf(" %s", cpu_feature_str(i));
	printf("\n");
	return 0;
}
