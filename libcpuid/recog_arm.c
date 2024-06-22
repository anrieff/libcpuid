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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libcpuid.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"
#include "recog_arm.h"

struct arm_feature_map_t {
	uint8_t highbit, lowbit;
	uint8_t value;
	cpu_feature_t feature;
};

struct arm_id_part {
	const int id;
	const char* name;
};

struct arm_hw_impl {
	const int32_t id;
	const cpu_vendor_t vendor;
	const struct arm_id_part *parts;
	const char *name;
};

/* Decoded PartNum for all implementers
   ID taken from lscpu-arm: https://github.com/util-linux/util-linux/blob/master/sys-utils/lscpu-arm.c
*/
static const struct arm_id_part arm_part[] = {
	{ 0x810, "ARM810" },
	{ 0x920, "ARM920" },
	{ 0x922, "ARM922" },
	{ 0x926, "ARM926" },
	{ 0x940, "ARM940" },
	{ 0x946, "ARM946" },
	{ 0x966, "ARM966" },
	{ 0xa20, "ARM1020" },
	{ 0xa22, "ARM1022" },
	{ 0xa26, "ARM1026" },
	{ 0xb02, "ARM11 MPCore" },
	{ 0xb36, "ARM1136" },
	{ 0xb56, "ARM1156" },
	{ 0xb76, "ARM1176" },
	{ 0xc05, "Cortex-A5" },
	{ 0xc07, "Cortex-A7" },
	{ 0xc08, "Cortex-A8" },
	{ 0xc09, "Cortex-A9" },
	{ 0xc0d, "Cortex-A17" },	/* Originally A12 */
	{ 0xc0f, "Cortex-A15" },
	{ 0xc0e, "Cortex-A17" },
	{ 0xc14, "Cortex-R4" },
	{ 0xc15, "Cortex-R5" },
	{ 0xc17, "Cortex-R7" },
	{ 0xc18, "Cortex-R8" },
	{ 0xc20, "Cortex-M0" },
	{ 0xc21, "Cortex-M1" },
	{ 0xc23, "Cortex-M3" },
	{ 0xc24, "Cortex-M4" },
	{ 0xc27, "Cortex-M7" },
	{ 0xc60, "Cortex-M0+" },
	{ 0xd01, "Cortex-A32" },
	{ 0xd02, "Cortex-A34" },
	{ 0xd03, "Cortex-A53" },
	{ 0xd04, "Cortex-A35" },
	{ 0xd05, "Cortex-A55" },
	{ 0xd06, "Cortex-A65" },
	{ 0xd07, "Cortex-A57" },
	{ 0xd08, "Cortex-A72" },
	{ 0xd09, "Cortex-A73" },
	{ 0xd0a, "Cortex-A75" },
	{ 0xd0b, "Cortex-A76" },
	{ 0xd0c, "Neoverse-N1" },
	{ 0xd0d, "Cortex-A77" },
	{ 0xd0e, "Cortex-A76AE" },
	{ 0xd13, "Cortex-R52" },
	{ 0xd15, "Cortex-R82" },
	{ 0xd16, "Cortex-R52+" },
	{ 0xd20, "Cortex-M23" },
	{ 0xd21, "Cortex-M33" },
	{ 0xd22, "Cortex-M55" },
	{ 0xd23, "Cortex-M85" },
	{ 0xd40, "Neoverse-V1" },
	{ 0xd41, "Cortex-A78" },
	{ 0xd42, "Cortex-A78AE" },
	{ 0xd43, "Cortex-A65AE" },
	{ 0xd44, "Cortex-X1" },
	{ 0xd46, "Cortex-A510" },
	{ 0xd47, "Cortex-A710" },
	{ 0xd48, "Cortex-X2" },
	{ 0xd49, "Neoverse-N2" },
	{ 0xd4a, "Neoverse-E1" },
	{ 0xd4b, "Cortex-A78C" },
	{ 0xd4c, "Cortex-X1C" },
	{ 0xd4d, "Cortex-A715" },
	{ 0xd4e, "Cortex-X3" },
	{ 0xd4f, "Neoverse-V2" },
	{ 0xd80, "Cortex-A520" },
	{ 0xd81, "Cortex-A720" },
	{ 0xd82, "Cortex-X4" },
	{ 0xd84, "Neoverse-V3" },
	{ 0xd8e, "Neoverse-N3" },
	{ -1, "unknown" },
};

static const struct arm_id_part brcm_part[] = {
	{ 0x0f, "Brahma-B15" },
	{ 0x100, "Brahma-B53" },
	{ 0x516, "ThunderX2" },
	{ -1, "unknown" },
};

static const struct arm_id_part dec_part[] = {
	{ 0xa10, "SA110" },
	{ 0xa11, "SA1100" },
	{ -1, "unknown" },
};

static const struct arm_id_part cavium_part[] = {
	{ 0x0a0, "ThunderX" },
	{ 0x0a1, "ThunderX-88XX" },
	{ 0x0a2, "ThunderX-81XX" },
	{ 0x0a3, "ThunderX-83XX" },
	{ 0x0af, "ThunderX2-99xx" },
	{ 0x0b0, "OcteonTX2" },
	{ 0x0b1, "OcteonTX2-98XX" },
	{ 0x0b2, "OcteonTX2-96XX" },
	{ 0x0b3, "OcteonTX2-95XX" },
	{ 0x0b4, "OcteonTX2-95XXN" },
	{ 0x0b5, "OcteonTX2-95XXMM" },
	{ 0x0b6, "OcteonTX2-95XXO" },
	{ 0x0b8, "ThunderX3-T110" },
	{ -1, "unknown" },
};

static const struct arm_id_part apm_part[] = {
	{ 0x000, "X-Gene" },
	{ -1, "unknown" },
};

static const struct arm_id_part qcom_part[] = {
	{ 0x001, "Oryon" },
	{ 0x00f, "Scorpion" },
	{ 0x02d, "Scorpion" },
	{ 0x04d, "Krait" },
	{ 0x06f, "Krait" },
	{ 0x201, "Kryo" },
	{ 0x205, "Kryo" },
	{ 0x211, "Kryo" },
	{ 0x800, "Falkor-V1/Kryo" },
	{ 0x801, "Kryo-V2" },
	{ 0x802, "Kryo-3XX-Gold" },
	{ 0x803, "Kryo-3XX-Silver" },
	{ 0x804, "Kryo-4XX-Gold" },
	{ 0x805, "Kryo-4XX-Silver" },
	{ 0xc00, "Falkor" },
	{ 0xc01, "Saphira" },
	{ -1, "unknown" },
};

static const struct arm_id_part samsung_part[] = {
	{ 0x001, "exynos-m1" },
	{ 0x002, "exynos-m3" },
	{ 0x003, "exynos-m4" },
	{ 0x004, "exynos-m5" },
	{ -1, "unknown" },
};

static const struct arm_id_part nvidia_part[] = {
	{ 0x000, "Denver" },
	{ 0x003, "Denver 2" },
	{ 0x004, "Carmel" },
	{ -1, "unknown" },
};

static const struct arm_id_part marvell_part[] = {
	{ 0x131, "Feroceon-88FR131" },
	{ 0x581, "PJ4/PJ4b" },
	{ 0x584, "PJ4B-MP" },
	{ -1, "unknown" },
};

static const struct arm_id_part apple_part[] = {
	{ 0x000, "Swift" },
	{ 0x001, "Cyclone" },
	{ 0x002, "Typhoon" },
	{ 0x003, "Typhoon/Capri" },
	{ 0x004, "Twister" },
	{ 0x005, "Twister/Elba/Malta" },
	{ 0x006, "Hurricane" },
	{ 0x007, "Hurricane/Myst" },
	{ 0x008, "Monsoon" },
	{ 0x009, "Mistral" },
	{ 0x00b, "Vortex" },
	{ 0x00c, "Tempest" },
	{ 0x00f, "Tempest-M9" },
	{ 0x010, "Vortex/Aruba" },
	{ 0x011, "Tempest/Aruba" },
	{ 0x012, "Lightning" },
	{ 0x013, "Thunder" },
	{ 0x020, "Icestorm-A14" },
	{ 0x021, "Firestorm-A14" },
	{ 0x022, "Icestorm-M1" },
	{ 0x023, "Firestorm-M1" },
	{ 0x024, "Icestorm-M1-Pro" },
	{ 0x025, "Firestorm-M1-Pro" },
	{ 0x026, "Thunder-M10" },
	{ 0x028, "Icestorm-M1-Max" },
	{ 0x029, "Firestorm-M1-Max" },
	{ 0x030, "Blizzard-A15" },
	{ 0x031, "Avalanche-A15" },
	{ 0x032, "Blizzard-M2" },
	{ 0x033, "Avalanche-M2" },
	{ 0x034, "Blizzard-M2-Pro" },
	{ 0x035, "Avalanche-M2-Pro" },
	{ 0x036, "Sawtooth-A16" },
	{ 0x037, "Everest-A16" },
	{ 0x038, "Blizzard-M2-Max" },
	{ 0x039, "Avalanche-M2-Max" },
	{ -1, "unknown" },
};

static const struct arm_id_part faraday_part[] = {
	{ 0x526, "FA526" },
	{ 0x626, "FA626" },
	{ -1, "unknown" },
};

static const struct arm_id_part intel_part[] = {
	{ 0x200, "i80200" },
	{ 0x210, "PXA250A" },
	{ 0x212, "PXA210A" },
	{ 0x242, "i80321-400" },
	{ 0x243, "i80321-600" },
	{ 0x290, "PXA250B/PXA26x" },
	{ 0x292, "PXA210B" },
	{ 0x2c2, "i80321-400-B0" },
	{ 0x2c3, "i80321-600-B0" },
	{ 0x2d0, "PXA250C/PXA255/PXA26x" },
	{ 0x2d2, "PXA210C" },
	{ 0x411, "PXA27x" },
	{ 0x41c, "IPX425-533" },
	{ 0x41d, "IPX425-400" },
	{ 0x41f, "IPX425-266" },
	{ 0x682, "PXA32x" },
	{ 0x683, "PXA930/PXA935" },
	{ 0x688, "PXA30x" },
	{ 0x689, "PXA31x" },
	{ 0xb11, "SA1110" },
	{ 0xc12, "IPX1200" },
	{ -1, "unknown" },
};

static const struct arm_id_part fujitsu_part[] = {
	{ 0x001, "A64FX" },
	{ -1, "unknown" },
};

static const struct arm_id_part hisi_part[] = {
	{ 0xd01, "TaiShan-v110" },	/* used in Kunpeng-920 SoC */
	{ 0xd02, "TaiShan-v120" },	/* used in Kirin 990A and 9000S SoCs */
	{ 0xd40, "Cortex-A76" },	/* HiSilicon uses this ID though advertises A76 */
	{ 0xd41, "Cortex-A77" },	/* HiSilicon uses this ID though advertises A77 */
	{ -1, "unknown" },
};

static const struct arm_id_part ampere_part[] = {
	{ 0xac3, "Ampere-1" },
	{ 0xac4, "Ampere-1a" },
	{ -1, "unknown" },
};

static const struct arm_id_part ft_part[] = {
	{ 0x303, "FTC310" },
	{ 0x660, "FTC660" },
	{ 0x661, "FTC661" },
	{ 0x662, "FTC662" },
	{ 0x663, "FTC663" },
	{ 0x664, "FTC664" },
	{ 0x862, "FTC862" },
	{ -1, "unknown" },
};

static const struct arm_id_part ms_part[] = {
	{ 0xd49, "Azure-Cobalt-100" },
	{ -1, "unknown" },
};

static const struct arm_id_part unknown_part[] = {
	{ -1, "unknown" },
};

/* Implementers list */
static const struct arm_hw_impl hw_implementer[] = {
	{ 0x41, VENDOR_ARM,       arm_part,     "ARM"                },
	{ 0x42, VENDOR_BROADCOM,  brcm_part,    "Broadcom"           },
	{ 0x43, VENDOR_CAVIUM,    cavium_part,  "Cavium"             },
	{ 0x44, VENDOR_DEC,       dec_part,     "DEC"                },
	{ 0x46, VENDOR_FUJITSU,   fujitsu_part, "FUJITSU"            },
	{ 0x48, VENDOR_HISILICON, hisi_part,    "HiSilicon"          },
	{ 0x49, VENDOR_INFINEON,  unknown_part, "Infineon"           },
	{ 0x4d, VENDOR_FREESCALE, unknown_part, "Motorola/Freescale" },
	{ 0x4e, VENDOR_NVIDIA,    nvidia_part,  "NVIDIA"             },
	{ 0x50, VENDOR_APM,       apm_part,     "APM"                },
	{ 0x51, VENDOR_QUALCOMM,  qcom_part,    "Qualcomm"           },
	{ 0x53, VENDOR_SAMSUNG,   samsung_part, "Samsung"            },
	{ 0x56, VENDOR_MARVELL,   marvell_part, "Marvell"            },
	{ 0x61, VENDOR_APPLE,     apple_part,   "Apple"              },
	{ 0x66, VENDOR_FARADAY,   faraday_part, "Faraday"            },
	{ 0x69, VENDOR_INTEL,     intel_part,   "Intel"              },
	{ 0x6d, VENDOR_MICROSOFT, ms_part,      "Microsoft"          },
	{ 0x70, VENDOR_PHYTIUM,   ft_part,      "Phytium"            },
	{ 0xc0, VENDOR_AMPERE,    ampere_part,  "Ampere"             },
	{   -1, VENDOR_UNKNOWN,   unknown_part, "unknown"            },
};

static const struct arm_hw_impl* get_cpu_implementer_from_code(uint8_t implementer)
{
	int i;

	for (i = 0; hw_implementer[i].id >= 0; i++) {
		if (hw_implementer[i].id == implementer)
			break;
	}

	/* This function returns an item with VENDOR_UNKNOWN if implementer is not found in the "hw_implementer" array */
	return &hw_implementer[i];
}

static const struct arm_hw_impl* get_cpu_implementer_from_vendor(cpu_vendor_t vendor)
{
	int i;

	for (i = 0; hw_implementer[i].id >= 0; i++) {
		if (hw_implementer[i].vendor == vendor)
			break;
	}

	/* This function returns an item with VENDOR_UNKNOWN if implementer is not found in the "hw_implementer" array */
	return &hw_implementer[i];
}

static const char* get_cpu_name(uint16_t part_num, const struct arm_hw_impl* hw_impl)
{
	int i;

	for (i = 0; hw_impl->parts[i].id >= 0; i++) {
		if (hw_impl->parts[i].id == part_num)
			break;
	}

	/* This function retuns "unknown" if PartNum is not found in the "parts" array */
	return hw_impl->parts[i].name;
}

static void match_arm_features(const struct arm_feature_map_t* matchtable, const char* reg_name, const int reg_number, uint32_t reg_value, struct cpu_id_t* data)
{
	int i;

	for (i = 0; matchtable[i].feature != -1; i++) {
		if (EXTRACTS_BITS(reg_value, matchtable[i].highbit, matchtable[i].lowbit) == matchtable[i].value) {
			data->flags[matchtable[i].feature] = 1;
			debugf(3, "Register %8s%i (0x%016X): match value %u for bits [%2u:%2u]\n", reg_name, reg_number, reg_value, matchtable[i].value, matchtable[i].highbit, matchtable[i].lowbit);
		}
	}
}

#define MAX_MATCHTABLE_ITEMS 32
#define MATCH_FEATURES_TABLE_WITH_RAW(reg) match_arm_features(matchtable_id_##reg[i], #reg, i, raw->arm_id_##reg[i], data)
static void load_arm_features(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int i;
	uint8_t mpam, mpam_frac;
	uint8_t mte, mte_frac, mtex;

	const struct arm_feature_map_t matchtable_id_aa64dfr[MAX_ARM_ID_AA64DFR_REGS][MAX_MATCHTABLE_ITEMS] = {
		[0] /* ID_AA64DFR0 */ = {
			{ 55, 52, 0b0001, CPU_FEATURE_BRBE },
			{ 51, 48, 0b0001, CPU_FEATURE_MTPMU },
			{ 47, 47, 0b0001, CPU_FEATURE_TRBE },
			{ 43, 40, 0b0001, CPU_FEATURE_TRF },
			{ 39, 36, 0b0000, CPU_FEATURE_DOUBLELOCK },
			{ 35, 32, 0b0001, CPU_FEATURE_SPE },
			{ 35, 32, 0b0010, CPU_FEATURE_SPEV1P1 },
			{ 35, 32, 0b0011, CPU_FEATURE_SPEV1P2 },
			{ 35, 32, 0b0100, CPU_FEATURE_SPEV1P3 },
			{ 35, 32, 0b0101, CPU_FEATURE_SPEV1P4 },
			{ 19, 16, 0b0001, CPU_FEATURE_PMUV3_SS },
			{ 11,  8, 0b0001, CPU_FEATURE_PMUV3 }, /* Performance Monitors Extension, PMUv3 implemented. */
			{ 11,  8, 0b0100, CPU_FEATURE_PMUV3P1 }, /* PMUv3 for Armv8.1 */
			{ 11,  8, 0b0101, CPU_FEATURE_PMUV3P4 }, /* PMUv3 for Armv8.4 */
			{ 11,  8, 0b0110, CPU_FEATURE_PMUV3P5 }, /* PMUv3 for Armv8.5 */
			{ 11,  8, 0b0111, CPU_FEATURE_PMUV3P7 }, /* PMUv3 for Armv8.7 */
			{ 11,  8, 0b1000, CPU_FEATURE_PMUV3P8 }, /* PMUv3 for Armv8.8 */
			{ 11,  8, 0b1001, CPU_FEATURE_PMUV3P9 }, /* PMUv3 for Armv8.9 */
			{  3,  0, 0b1000, CPU_FEATURE_DEBUGV8P2 }, /* Armv8.2 debug architecture */
			{  3,  0, 0b1001, CPU_FEATURE_DEBUGV8P4 }, /* Armv8.4 debug architecture */
			{  3,  0, 0b1010, CPU_FEATURE_DEBUGV8P8 }, /* Armv8.8 debug architecture */
			{  3,  0, 0b1011, CPU_FEATURE_DEBUGV8P9 }, /* Armv8.9 debug architecture */
			{ -1, -1,     -1, -1 }
		},
		[1] /* ID_AA64DFR1 */ = {
			{ 55, 52, 0b0001, CPU_FEATURE_SPE_DPFZS },
			{ 39, 36, 0b0001, CPU_FEATURE_PMUV3_ICNTR },
			{ 35, 32, 0b0001, CPU_FEATURE_SPMU },
			{ -1, -1,     -1, -1 }
		}
	};

	const struct arm_feature_map_t matchtable_id_aa64isar[MAX_ARM_ID_AA64ISAR_REGS][MAX_MATCHTABLE_ITEMS] = {
		[0] /* ID_AA64ISAR0 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_RNG },
			{ 59, 56, 0b0001, CPU_FEATURE_TLBIOS }, /* Outer Shareable and TLB range maintenance instructions are not implemented */
			{ 59, 56, 0b0010, CPU_FEATURE_TLBIOS }, /* Outer Shareable TLB maintenance instructions are implemented */
			{ 59, 56, 0b0010, CPU_FEATURE_TLBIRANGE }, /* Outer Shareable TLB maintenance instructions are implemented */
			{ 55, 52, 0b0001, CPU_FEATURE_FLAGM },
			{ 55, 52, 0b0010, CPU_FEATURE_FLAGM2 },
			{ 51, 48, 0b0001, CPU_FEATURE_FHM },
			{ 47, 44, 0b0001, CPU_FEATURE_DOTPROD },
			{ 43, 40, 0b0001, CPU_FEATURE_SM4 },
			{ 39, 36, 0b0001, CPU_FEATURE_SM3 },
			{ 35, 32, 0b0001, CPU_FEATURE_SHA3 },
			{ 31, 28, 0b0001, CPU_FEATURE_RDM },
			{ 27, 24, 0b0001, CPU_FEATURE_TME },
			{ 23, 20, 0b0010, CPU_FEATURE_LSE },
			{ 19, 16, 0b0001, CPU_FEATURE_CRC32 },
			{ 15, 12, 0b0001, CPU_FEATURE_SHA256 },
			{ 15, 12, 0b0010, CPU_FEATURE_SHA512 },
			{ 11,  8, 0b0001, CPU_FEATURE_SHA1 },
			{  7,  4, 0b0001, CPU_FEATURE_AES },
			{  7,  4, 0b0010, CPU_FEATURE_PMULL },
			{ -1, -1,     -1, -1 }
		},
		[1] /* ID_AA64ISAR1 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_LS64 },
			{ 63, 60, 0b0010, CPU_FEATURE_LS64_V },
			{ 63, 60, 0b0011, CPU_FEATURE_LS64_ACCDATA },
			{ 59, 56, 0b0001, CPU_FEATURE_XS },
			{ 55, 52, 0b0001, CPU_FEATURE_I8MM },
			{ 51, 48, 0b0001, CPU_FEATURE_DGH },
			{ 47, 44, 0b0001, CPU_FEATURE_BF16 },
			{ 47, 44, 0b0010, CPU_FEATURE_EBF16 },
			{ 43, 40, 0b0001, CPU_FEATURE_SPECRES },
			{ 43, 40, 0b0010, CPU_FEATURE_SPECRES2 },
			{ 39, 36, 0b0001, CPU_FEATURE_SB },
			{ 35, 32, 0b0001, CPU_FEATURE_FRINTTS },
			{ 31, 28, 0b0001, CPU_FEATURE_PACIMP },
			{ 27, 24, 0b0001, CPU_FEATURE_PACQARMA5 },
			{ 23, 20, 0b0001, CPU_FEATURE_LRCPC },
			{ 23, 20, 0b0010, CPU_FEATURE_LRCPC2 },
			{ 23, 20, 0b0011, CPU_FEATURE_LRCPC3 },
			{ 19, 16, 0b0001, CPU_FEATURE_FCMA },
			{ 15, 12, 0b0001, CPU_FEATURE_JSCVT },
			{ 11,  8, 0b0001, CPU_FEATURE_PAUTH },
			{ 11,  8, 0b0010, CPU_FEATURE_EPAC },
			{ 11,  8, 0b0011, CPU_FEATURE_PAUTH2 },
			{ 11,  8, 0b0100, CPU_FEATURE_FPAC },
			{ 11,  8, 0b0101, CPU_FEATURE_FPACCOMBINE },
			{  7,  4, 0b0001, CPU_FEATURE_PAUTH },
			{  7,  4, 0b0010, CPU_FEATURE_EPAC },
			{  7,  4, 0b0011, CPU_FEATURE_PAUTH2 },
			{  7,  4, 0b0100, CPU_FEATURE_FPAC },
			{  7,  4, 0b0101, CPU_FEATURE_FPACCOMBINE },
			{  3,  0, 0b0001, CPU_FEATURE_DPB },
			{  3,  0, 0b0010, CPU_FEATURE_DPB2 },
			{ -1, -1,     -1, -1 }
		},
		[2] /* ID_AA64ISAR2 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_ATS1A },
			{ 55, 52, 0b0001, CPU_FEATURE_CSSC },
			{ 51, 48, 0b0001, CPU_FEATURE_RPRFM },
			{ 43, 40, 0b0001, CPU_FEATURE_PRFMSLC },
			{ 31, 28, 0b0001, CPU_FEATURE_CLRBHB },
			{ 27, 24, 0b0001, CPU_FEATURE_CONSTPACFIELD },
			{ 23, 20, 0b0001, CPU_FEATURE_HBC },
			{ 19, 16, 0b0001, CPU_FEATURE_MOPS },
			{ 15, 12, 0b0001, CPU_FEATURE_PAUTH },
			{ 15, 12, 0b0010, CPU_FEATURE_EPAC },
			{ 15, 12, 0b0011, CPU_FEATURE_PAUTH2 },
			{ 15, 12, 0b0100, CPU_FEATURE_FPAC },
			{ 15, 12, 0b0101, CPU_FEATURE_FPACCOMBINE },
			{ 11,  8, 0b0001, CPU_FEATURE_PACQARMA3 },
			{  7,  4, 0b0001, CPU_FEATURE_RPRES },
			{  3,  0, 0b0010, CPU_FEATURE_WFXT },
			{ -1, -1,     -1, -1 }
		}
	};

	const struct arm_feature_map_t matchtable_id_aa64mmfr[MAX_ARM_ID_AA64MMFR_REGS][MAX_MATCHTABLE_ITEMS] = {
		[0] /* ID_AA64MMFR0 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_ECV },
			{ 63, 60, 0b0010, CPU_FEATURE_ECV },
			{ 59, 56, 0b0001, CPU_FEATURE_FGT },
			{ 59, 56, 0b0010, CPU_FEATURE_FGT2 },
			{ 47, 44, 0b0001, CPU_FEATURE_EXS },
			{ 31, 28, 0b0001, CPU_FEATURE_LPA2 },
			{ 23, 20, 0b0001, CPU_FEATURE_LPA2 },
			{ 19, 16, 0b0001, CPU_FEATURE_MIXEDEND },
			{  7,  4, 0b0010, CPU_FEATURE_ASID16 },
			{  3,  0, 0b0110, CPU_FEATURE_LPA },
			{ -1, -1,     -1, -1 }
		},
		[1] /* ID_AA64MMFR1 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_ECBHB },
			{ 59, 56, 0b0001, CPU_FEATURE_CMOW },
			{ 55, 52, 0b0001, CPU_FEATURE_TIDCP1 },
			{ 47, 44, 0b0001, CPU_FEATURE_AFP },
			{ 43, 40, 0b0001, CPU_FEATURE_HCX },
			{ 39, 36, 0b0010, CPU_FEATURE_ETS2 },
			{ 35, 32, 0b0001, CPU_FEATURE_TWED },
			{ 31, 28, 0b0001, CPU_FEATURE_XNX },
			{ 23, 20, 0b0001, CPU_FEATURE_PAN },
			{ 23, 20, 0b0010, CPU_FEATURE_PAN2 },
			{ 23, 20, 0b0011, CPU_FEATURE_PAN3 },
			{ 19, 16, 0b0001, CPU_FEATURE_LOR },
			{ 15, 12, 0b0001, CPU_FEATURE_HPDS },
			{ 15, 12, 0b0010, CPU_FEATURE_HPDS2 },
			{ 11,  8, 0b0001, CPU_FEATURE_VHE },
			{  7,  4, 0b0010, CPU_FEATURE_VMID16 },
			{  3,  0, 0b0001, CPU_FEATURE_HAFDBS },
			{  3,  0, 0b0010, CPU_FEATURE_HAFDBS }, /* as 0b0001, and adds support for hardware update of the Access flag for Block and Page descriptors */
			{  3,  0, 0b0011, CPU_FEATURE_HAFT },
			{ -1, -1,     -1, -1 }
		},
		[2] /* ID_AA64MMFR2 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_E0PD },
			{ 59, 56, 0b0001, CPU_FEATURE_EVT }, /* HCR_EL2.{TOCU, TICAB, TID4} traps are supported. HCR_EL2.{TTLBOS,TTLBIS} traps are not supported */
			{ 59, 56, 0b0010, CPU_FEATURE_EVT }, /* HCR_EL2.{TTLBOS, TTLBIS, TOCU, TICAB, TID4} traps are supported */
			{ 55, 52, 0b0000, CPU_FEATURE_BBM }, /* Level 0 support for changing block size is supported */
			{ 55, 52, 0b0001, CPU_FEATURE_BBM }, /* Level 1 support for changing block size is supported */
			{ 55, 52, 0b0010, CPU_FEATURE_BBM }, /* Level 2 support for changing block size is supported */
			{ 51, 48, 0b0001, CPU_FEATURE_TTL },
			{ 43, 40, 0b0001, CPU_FEATURE_S2FWB },
			{ 39, 36, 0b0001, CPU_FEATURE_IDST },
			{ 35, 32, 0b0001, CPU_FEATURE_LSE2 },
			{ 31, 28, 0b0001, CPU_FEATURE_TTST },
			{ 23, 20, 0b0001, CPU_FEATURE_CCIDX },
			{ 19, 16, 0b0001, CPU_FEATURE_LVA },
			{ 15, 12, 0b0001, CPU_FEATURE_IESB },
			{ 11,  8, 0b0001, CPU_FEATURE_LSMAOC },
			{  7,  4, 0b0001, CPU_FEATURE_UAO },
			{  3,  0, 0b0001, CPU_FEATURE_TTCNP },
			{ -1, -1,     -1, -1 }
		},
		[3] /* ID_AA64MMFR3 */ = {
			{ 59, 56, 0b0010, CPU_FEATURE_ADERR },
			{ 55, 52, 0b0010, CPU_FEATURE_ADERR },
			{ 47, 44, 0b0010, CPU_FEATURE_ANERR },
			{ 43, 40, 0b0010, CPU_FEATURE_ANERR },
			{ 27, 24, 0b0001, CPU_FEATURE_AIE },
			{ 23, 20, 0b0001, CPU_FEATURE_S2POE },
			{ 19, 16, 0b0001, CPU_FEATURE_S1POE },
			{ 15, 12, 0b0001, CPU_FEATURE_S2PIE },
			{ 11,  8, 0b0001, CPU_FEATURE_S1PIE },
			{  7,  4, 0b0001, CPU_FEATURE_SCTLR2 },
			{  3,  0, 0b0001, CPU_FEATURE_TCR2 },
			{ -1, -1,     -1, -1 }
		},
		[4] /* ID_AA64MMFR4 */ = {
			{ -1, -1,     -1, -1 }
		}
	};

	const struct arm_feature_map_t matchtable_id_aa64pfr[MAX_ARM_ID_AA64PFR_REGS][MAX_MATCHTABLE_ITEMS] = {
		[0] /* ID_AA64PFR0 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_CSV3 },
			{ 59, 56, 0b0001, CPU_FEATURE_CSV2 },
			{ 59, 56, 0b0010, CPU_FEATURE_CSV2_2 },
			{ 59, 56, 0b0011, CPU_FEATURE_CSV2_3 },
			{ 55, 52, 0b0001, CPU_FEATURE_RME },
			{ 51, 48, 0b0001, CPU_FEATURE_DIT },
			{ 47, 44, 0b0001, CPU_FEATURE_AMUV1 },
			{ 47, 44, 0b0010, CPU_FEATURE_AMUV1P1 },
			{ 39, 36, 0b0001, CPU_FEATURE_SEL2 },
			{ 35, 32, 0b0001, CPU_FEATURE_SVE },
			{ 31, 28, 0b0001, CPU_FEATURE_RAS },
			{ 31, 28, 0b0010, CPU_FEATURE_DOUBLEFAULT },
			{ 31, 28, 0b0010, CPU_FEATURE_RASV1P1 },
			{ 31, 28, 0b0011, CPU_FEATURE_RASV2 },
			{ 23, 20, 0b0000, CPU_FEATURE_ADVSIMD },
			{ 23, 20, 0b0001, CPU_FEATURE_ADVSIMD }, /* as for 0b0000, and also includes support for half-precision floating-point arithmetic */
			{ 19, 16, 0b0000, CPU_FEATURE_FP },
			{ 19, 16, 0b0001, CPU_FEATURE_FP16 },
			{ -1, -1,     -1, -1 }
		},
		[1] /* ID_AA64PFR1 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_PFAR },
			{ 59, 56, 0b0001, CPU_FEATURE_DOUBLEFAULT2 },
			{ 51, 48, 0b0001, CPU_FEATURE_THE },
			{ 39, 36, 0b0001, CPU_FEATURE_NMI },
			{ 35, 32, 0b0001, CPU_FEATURE_CSV2_1P1 },
			{ 35, 32, 0b0010, CPU_FEATURE_CSV2_1P2 },
			{ 31, 28, 0b0001, CPU_FEATURE_RNG_TRAP },
			{ 27, 24, 0b0001, CPU_FEATURE_SME },
			{ 27, 24, 0b0010, CPU_FEATURE_SME }, /* As 0b0001, plus the SME2 ZT0 register */
			{ 11,  8, 0b0001, CPU_FEATURE_MTE },
			{ 11,  8, 0b0010, CPU_FEATURE_MTE2 },
			{ 11,  8, 0b0011, CPU_FEATURE_MTE3 },
			{ 11,  8, 0b0011, CPU_FEATURE_MTE_ASYM_FAULT }, /* FEAT_MTE3 is implemented if and only if FEAT_MTE_ASYM_FAULT is implemented */
			{  7,  4, 0b0001, CPU_FEATURE_SSBS },
			{  7,  4, 0b0010, CPU_FEATURE_SSBS2 },
			{  3,  0, 0b0001, CPU_FEATURE_BTI },
			{ -1, -1,     -1, -1 }
		},
		[2] /* ID_AA64PFR2 */ = {
			{ 11,  8, 0b0001, CPU_FEATURE_MTE_TAGGED_FAR },
			{  7,  4, 0b0001, CPU_FEATURE_MTE_STORE_ONLY },
			{  3,  0, 0b0001, CPU_FEATURE_MTE_PERM },
			{ -1, -1,     -1, -1 }
		}
	};

	const struct arm_feature_map_t matchtable_id_aa64smfr[MAX_ARM_ID_AA64SMFR_REGS][MAX_MATCHTABLE_ITEMS] = {
		[0] /* ID_AA64SMFR0 */ = {
			{ 63, 63,    0b1, CPU_FEATURE_SME_FA64 },
			{ 55, 52, 0b1111, CPU_FEATURE_SME_I16I64 },
			{ 48, 48,    0b1, CPU_FEATURE_SME_F64F64 },
			{ -1, -1,     -1, -1 }
		},
	};

	const struct arm_feature_map_t matchtable_id_aa64zfr[MAX_ARM_ID_AA64ZFR_REGS][MAX_MATCHTABLE_ITEMS] = {
		[0] /* ID_AA64ZFR0 */ = {
			{ 59, 56, 0b0001, CPU_FEATURE_F64MM },
			{ 55, 52, 0b0001, CPU_FEATURE_F32MM },
			{ 47, 44, 0b0001, CPU_FEATURE_I8MM },
			{ 43, 40, 0b0001, CPU_FEATURE_SVE_SM4 },
			{ 35, 32, 0b0001, CPU_FEATURE_SVE_SHA3 },
			{ 23, 20, 0b0001, CPU_FEATURE_BF16 },
			{ 23, 20, 0b0010, CPU_FEATURE_EBF16 },
			{ 19, 16, 0b0001, CPU_FEATURE_SVE_BITPERM },
			{  7,  4, 0b0001, CPU_FEATURE_SVE_AES },
			{  7,  4, 0b0010, CPU_FEATURE_SVE_PMULL128 },
			{  3,  0, 0b0001, CPU_FEATURE_SVE2 },
			//{  3,  0, 0b0001, CPU_FEATURE_SME },
			//{  3,  0, 0b0010, CPU_FEATURE_SME2P1 },
			//{  3,  0, 0b0010, CPU_FEATURE_SVE2P1 },
			{ -1, -1,     -1, -1 }
		},
	};

	for (i = 0; i < MAX_ARM_ID_AA64DFR_REGS; i++)
		MATCH_FEATURES_TABLE_WITH_RAW(aa64dfr);

	for (i = 0; i < MAX_ARM_ID_AA64ISAR_REGS; i++)
		MATCH_FEATURES_TABLE_WITH_RAW(aa64isar);

	for (i = 0; i < MAX_ARM_ID_AA64MMFR_REGS; i++)
		MATCH_FEATURES_TABLE_WITH_RAW(aa64mmfr);

	for (i = 0; i < MAX_ARM_ID_AA64PFR_REGS; i++)
		MATCH_FEATURES_TABLE_WITH_RAW(aa64pfr);

	for (i = 0; i < MAX_ARM_ID_AA64SMFR_REGS; i++)
		MATCH_FEATURES_TABLE_WITH_RAW(aa64smfr);

	for (i = 0; i < MAX_ARM_ID_AA64ZFR_REGS; i++)
		MATCH_FEATURES_TABLE_WITH_RAW(aa64zfr);

	/* FEAT_PAuth, Pointer authentication
	When FEAT_PAuth is implemented, one of the following must be true:
	- Exactly one of the PAC algorithms is implemented.
	- If the PACGA instruction and other Pointer authentication instructions use different PAC algorithms, exactly two PAC algorithms are implemented.
	The PAC algorithm features are:
	- FEAT_PACQARMA5.
	- FEAT_PACIMP.
	- FEAT_PACQARMA3. */
	if (data->flags[CPU_FEATURE_PACIMP] || data->flags[CPU_FEATURE_PACQARMA3] || data->flags[CPU_FEATURE_PACQARMA5])
		data->flags[CPU_FEATURE_PAUTH] = 1;

	/* FEAT_MPAM, Memory Partitioning and Monitoring Extension
	MPAM Extension version: MPAM | MPAM_frac
	Not implemented: 0b0000 | 0b0000
	v0.1 is implemented: 0b0000 | 0b0001
	v1.0 is implemented: 0b0001 | 0b0000
	v1.1 is implemented: 0b0001 | 0b0001 */
	mpam      = EXTRACTS_BITS(raw->arm_id_aa64pfr[0], 43, 40);
	mpam_frac = EXTRACTS_BITS(raw->arm_id_aa64pfr[1], 19, 16);
	if ((mpam != 0b0000) || (mpam_frac != 0b0000)) {
		data->flags[CPU_FEATURE_MPAM] = 1;
		if ((mpam == 0b0000) && (mpam_frac == 0b0001))
			data->flags[CPU_FEATURE_MPAMV0P1] = 1;
		else if ((mpam == 0b0001) && (mpam_frac == 0b0001))
			data->flags[CPU_FEATURE_MPAMV1P1] = 1;
		debugf(2, "MPAM Extension version is %u.%u\n", mpam, mpam_frac);
	}

	/* FEAT_MTE4, Enhanced Memory Tagging Extension */
	mte = EXTRACTS_BITS(raw->arm_id_aa64pfr[1], 11,  8);
	if (mte >= 0b0010) {
		mte_frac = EXTRACTS_BITS(raw->arm_id_aa64pfr[1], 43, 40); /* This field is valid only if ID_AA64PFR1_EL1.MTE >= 0b0010 */
		if (mte_frac == 0b0000)
			data->flags[CPU_FEATURE_MTE_ASYNC] = 1;

		mtex = EXTRACTS_BITS(raw->arm_id_aa64pfr[1], 55, 52); /* This field is valid only if ID_AA64PFR1_EL1.MTE >= 0b0010 */
		if (mtex == 0b0001) {
			data->flags[CPU_FEATURE_MTE_CANONICAL_TAGS]  = 1;
			data->flags[CPU_FEATURE_MTE_NO_ADDRESS_TAGS] = 1;
		}
	}
	/* If FEAT_MTE4 is implemented, then FEAT_MTE_CANONICAL_TAGS,
	FEAT_MTE_NO_ADDRESS_TAGS, FEAT_MTE_TAGGED_FAR, and
	FEAT_MTE_STORE_ONLY are implemented. */
	if (data->flags[CPU_FEATURE_MTE_CANONICAL_TAGS] &&
	    data->flags[CPU_FEATURE_MTE_NO_ADDRESS_TAGS] &&
	    data->flags[CPU_FEATURE_MTE_TAGGED_FAR] &&
	    data->flags[CPU_FEATURE_MTE_STORE_ONLY])
		data->flags[CPU_FEATURE_MTE4] = 1;
}
#undef MAX_MATCHTABLE_ITEMS
#undef MATCH_FEATURES_TABLE_WITH_RAW

int cpuid_identify_arm(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	/* Basic values extraction
	   Based on "Arm Architecture Reference Manual for A-profile architecture", section D23.2.122 "MIDR_EL1, Main ID Register"
	*/
	data->implementer          = EXTRACTS_BITS(raw->arm_midr, 31, 24);
	data->variant              = EXTRACTS_BITS(raw->arm_midr, 23, 20);
	data->architecture_version = EXTRACTS_BITS(raw->arm_midr, 19, 16);
	data->part_num             = EXTRACTS_BITS(raw->arm_midr, 15,  4);
	data->revision             = EXTRACTS_BITS(raw->arm_midr,  3,  0);
	data->purpose              = cpuid_identify_purpose_arm(raw);

	/* Values decoding */
	const struct arm_hw_impl* hw_impl = get_cpu_implementer_from_code(data->implementer);
	const char* cpu_name = get_cpu_name(data->part_num, hw_impl);
	data->vendor = hw_impl->vendor;
	strncpy(data->vendor_str, hw_impl->name, VENDOR_STR_MAX);
	strncpy(data->brand_str,  cpu_name,      BRAND_STR_MAX);
	load_arm_features(raw, data);

	return 0;
}

void cpuid_get_list_arm(cpu_vendor_t vendor, struct cpu_list_t* list)
{
	int i, j, n, good, sub_count, total_count;
	char **tmp_names = NULL;
	const struct arm_hw_impl* hw_impl = get_cpu_implementer_from_vendor(vendor);

	/* Get item count in hw_impl->parts */
	sub_count = 0;
	for (i = 0; hw_impl->parts[i].id >= 0; i++)
		sub_count++;
	total_count = list->num_entries + sub_count;
	if (sub_count == 0)
		return;

	/* Grow current list */
	n = list->num_entries;
	tmp_names = (char**) realloc(list->names, sizeof(char*) * total_count);
	if (!tmp_names) { /* Memory allocation failure */
		cpuid_set_error(ERR_NO_MEM);
		return;
	}
	list->names = tmp_names;

	/* Append item to list */
	for (i = 0; i < sub_count; i++) {
		if (strstr(hw_impl->parts[i].name, "Unknown")) continue;
		good = 1;
		for (j = n - 1; j >= 0; j--)
			if (!strcmp(list->names[j], hw_impl->parts[i].name)) {
				good = 0;
				break;
			}
		if (!good) continue;
#if defined(_MSC_VER)
		list->names[n] = _strdup(hw_impl->parts[i].name);
#else
		list->names[n] = strdup(hw_impl->parts[i].name);
#endif
		if (!list->names[n]) { /* Memory allocation failure */
			cpuid_set_error(ERR_NO_MEM);
			list->num_entries = 0;
			for (j = 0; j < n; j++) {
				free(list->names[j]);
			}
			free(list->names);
			list->names = NULL;
			return;
		}
		n++;
	}
	list->num_entries = n;
}

cpu_purpose_t cpuid_identify_purpose_arm(struct cpu_raw_data_t* raw)
{
	const uint8_t implementer         = EXTRACTS_BITS(raw->arm_midr, 31, 24);
	const uint16_t part_num           = EXTRACTS_BITS(raw->arm_midr, 15,  4);
	const struct arm_hw_impl* hw_impl = get_cpu_implementer_from_code(implementer);
	const char* cpu_name              = get_cpu_name(part_num, hw_impl);

	/* ARM big.LITTLE Typical Processor Combinations: https://www.arm.com/technologies/big-little */
	if (match_pattern(cpu_name, "Cortex-X[012356789]"))
		return PURPOSE_U_PERFORMANCE;
	else if (match_pattern(cpu_name, "Cortex-A[67][012356789]") || match_pattern(cpu_name, "Cortex-A[5][6789]"))
		return PURPOSE_PERFORMANCE;
	else if (match_pattern(cpu_name, "Cortex-A[5][012345]") || match_pattern(cpu_name, "Cortex-A[3][0123456789]"))
		return PURPOSE_EFFICIENCY;

	return PURPOSE_GENERAL;
}
