"""
The definition of C structures used by the libcpuid library.
"""

from ctypes import (
    c_int,
    c_int32,
    c_uint8,
    c_uint32,
    c_uint64,
    c_char,
    c_char_p,
    POINTER,
    Structure,
)
from . import consts

CPUArchitecture = c_int32
CPUVendor = c_int32
CPUPurpose = c_int32
HypervisorVendor = c_int32
CPUSGXFeature = c_uint8
CPUHint = c_uint8
CPUFeature = c_uint8
RegisterArray = c_uint32 * consts.NUM_REGS


class CPURawData(Structure):  # pylint: disable=too-few-public-methods
    """Mimics the 'cpu_raw_data_t' structure."""

    _fields_ = [
        ("basic_cpuid", RegisterArray * consts.MAX_CPUID_LEVEL),
        ("ext_cpuid", RegisterArray * consts.MAX_EXT_CPUID_LEVEL),
        ("intel_fn4", RegisterArray * consts.MAX_INTELFN4_LEVEL),
        ("intel_fn11", RegisterArray * consts.MAX_INTELFN11_LEVEL),
        ("intel_fn12h", RegisterArray * consts.MAX_INTELFN12H_LEVEL),
        ("intel_fn14h", RegisterArray * consts.MAX_INTELFN14H_LEVEL),
        ("amd_fn8000001dh", RegisterArray * consts.MAX_AMDFN8000001DH_LEVEL),
    ]


class CPUAffinityMask(Structure):  # pylint: disable=too-few-public-methods
    """Mimics the 'cpu_affinity_mask_t' structure."""

    _fields_ = [("bits", c_uint8 * consts.MASK_SETSIZE)]


class CPUSGX(Structure):  # pylint: disable=too-few-public-methods
    """Mimics the 'cpu_sgx_t' structure."""

    _fields_ = [
        ("present", c_uint32),
        ("max_enclave_32bit", c_uint8),
        ("max_enclave_64bit", c_uint8),
        ("flags", CPUSGXFeature * consts.SGX_FLAGS_MAX),
        ("num_epc_sections", c_int),
        ("misc_select", c_uint32),
        ("secs_attributes", c_uint64),
        ("secs_xfrm", c_uint64),
    ]


class CPUID(Structure):  # pylint: disable=too-few-public-methods
    """Mimics the 'cpu_id_t' structure."""

    _fields_ = [
        ("architecture", CPUArchitecture),
        ("vendor_str", c_char * consts.VENDOR_STR_MAX),
        ("brand_str", c_char * consts.BRAND_STR_MAX),
        ("vendor", CPUVendor),
        ("flags", CPUFeature * consts.CPU_FLAGS_MAX),
        ("family", c_int32),
        ("model", c_int32),
        ("stepping", c_int32),
        ("ext_family", c_int32),
        ("ext_model", c_int32),
        ("num_cores", c_int32),
        ("num_logical_cpus", c_int32),
        ("total_logical_cpus", c_int32),
        ("l1_data_cache", c_int32),
        ("l1_instruction_cache", c_int32),
        ("l2_cache", c_int32),
        ("l3_cache", c_int32),
        ("l4_cache", c_int32),
        ("l1_assoc", c_int32),
        ("l1_data_assoc", c_int32),
        ("l1_instruction_assoc", c_int32),
        ("l2_assoc", c_int32),
        ("l3_assoc", c_int32),
        ("l4_assoc", c_int32),
        ("l1_cacheline", c_int32),
        ("l1_data_cacheline", c_int32),
        ("l1_instruction_cacheline", c_int32),
        ("l2_cacheline", c_int32),
        ("l3_cacheline", c_int32),
        ("l4_cacheline", c_int32),
        ("l1_data_instances", c_int32),
        ("l1_instruction_instances", c_int32),
        ("l2_instances", c_int32),
        ("l3_instances", c_int32),
        ("l4_instances", c_int32),
        ("cpu_codename", c_char * consts.CODENAME_STR_MAX),
        ("sse_size", c_int32),
        ("detection_hints", CPUHint * consts.CPU_HINTS_MAX),
        ("sgx", CPUSGX),
        ("affinity_mask", CPUAffinityMask),
        ("purpose", CPUPurpose),
    ]


class CPUList(Structure):  # pylint: disable=too-few-public-methods
    """Mimics the 'cpu_list_t' structure."""

    _fields_ = [
        ("num_entries", c_int),
        ("names", POINTER(c_char_p)),
    ]
