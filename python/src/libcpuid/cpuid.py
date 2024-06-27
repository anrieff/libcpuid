"""
The main module, providing a class that holds CPU information.
"""

from typing import Optional
from libcpuid import enums, cpusgx
from libcpuid._utils import c_string_to_str, optional_int
from libcpuid._libcpuid_cffi import (  # pylint: disable=no-name-in-module, import-error
    lib,
    ffi,
)


class CPUID:  # pylint: disable=too-many-public-methods
    """
    Holds all acquirable information about a CPU.

    Use :func:`libcpuid.cpu_identify` to create an instance of this class:

    >>> cpuid = libcpuid.cpu_identify()
    >>> print(cpuid.architecture.name)
    """

    def __init__(self, raw):
        self._raw = raw

    @property
    def architecture(self) -> enums.CPUArchitecture:
        """The CPU Architecture."""
        try:
            return enums.CPUArchitecture(self._raw.architecture)
        except ValueError:
            return enums.CPUArchitecture.UNKNOWN

    @property
    def vendor_str(self) -> str:
        """The CPU vendor string, e.g., :const:`'GenuineIntel'`."""
        return c_string_to_str(self._raw.vendor_str)

    @property
    def brand_str(self) -> str:
        """The CPU brand string, e.g., :const:`'Intel(R) Xeon(TM) CPU 2.40GHz'`."""
        return c_string_to_str(self._raw.brand_str)

    @property
    def vendor(self) -> enums.CPUVendor:
        """The CPU vendor."""
        try:
            return enums.CPUVendor(self._raw.vendor)
        except ValueError:
            return enums.CPUVendor.UNKNOWN

    @property
    def flags(self) -> list[enums.CPUFeature]:
        """List of CPU features"""
        return [flag for flag in enums.CPUFeature if self._raw.flags[flag]]

    @property
    def family(self) -> int:
        """The CPU family number."""
        return self._raw.family

    @property
    def model(self) -> int:
        """The CPU model number."""
        return self._raw.model

    @property
    def stepping(self) -> int:
        """The CPU stepping."""
        return self._raw.stepping

    @property
    def ext_family(self) -> int:
        """The CPU display family number."""
        return self._raw.ext_family

    @property
    def ext_model(self) -> int:
        """The CPU display model number."""
        return self._raw.ext_model

    @property
    def num_cores(self) -> int:
        """The number of CPU cores."""
        return self._raw.num_cores

    @property
    def num_logical_cpus(self) -> int:
        """The number of logical CPU cores."""
        return self._raw.num_logical_cpus

    @property
    def total_logical_cpus(self) -> int:
        """The total number of logical processors."""
        return self._raw.total_logical_cpus

    @property
    def l1_data_cache(self) -> Optional[int]:
        """L1 data cache size in KB. :const:`None` if not determined."""
        return optional_int(self._raw.l1_data_cache)

    @property
    def l1_instruction_cache(self) -> Optional[int]:
        """L1 instruction cache size in KB. :const:`None` if not determined."""
        return optional_int(self._raw.l1_instruction_cache)

    @property
    def l2_cache(self) -> Optional[int]:
        """L2 cache size in KB. :const:`None` if not determined."""
        return optional_int(self._raw.l2_cache)

    @property
    def l3_cache(self) -> Optional[int]:
        """L3 cache size in KB. :const:`None` if not determined."""
        return optional_int(self._raw.l3_cache)

    @property
    def l4_cache(self) -> Optional[int]:
        """L4 cache size in KB. :const:`None` if not determined."""
        return optional_int(self._raw.l4_cache)

    @property
    def l1_data_assoc(self) -> Optional[int]:
        """L1 data cache associativity. :const:`None` if not determined."""
        return optional_int(self._raw.l1_data_assoc)

    @property
    def l1_instruction_assoc(self) -> Optional[int]:
        """L1 instruction cache associativity. :const:`None` if not determined."""
        return optional_int(self._raw.l1_instruction_assoc)

    @property
    def l2_assoc(self) -> Optional[int]:
        """L2 cache associativity. :const:`None` if not determined."""
        return optional_int(self._raw.l2_assoc)

    @property
    def l3_assoc(self) -> Optional[int]:
        """L3 cache associativity. :const:`None` if not determined."""
        return optional_int(self._raw.l3_assoc)

    @property
    def l4_assoc(self) -> Optional[int]:
        """L4 cache associativity. :const:`None` if not determined."""
        return optional_int(self._raw.l4_assoc)

    @property
    def l1_data_cacheline(self) -> Optional[int]:
        """L1 data cache line size. :const:`None` if not determined."""
        return optional_int(self._raw.l1_data_cacheline)

    @property
    def l1_instruction_cacheline(self) -> Optional[int]:
        """L1 instruction cache line size. :const:`None` if not determined."""
        return optional_int(self._raw.l1_instruction_cacheline)

    @property
    def l2_cacheline(self) -> Optional[int]:
        """L2 cache line size. :const:`None` if not determined."""
        return optional_int(self._raw.l2_cacheline)

    @property
    def l3_cacheline(self) -> Optional[int]:
        """L3 cache line size. :const:`None` if not determined."""
        return optional_int(self._raw.l3_cacheline)

    @property
    def l4_cacheline(self) -> Optional[int]:
        """L4 cache line size. :const:`None` if not determined."""
        return optional_int(self._raw.l4_cacheline)

    @property
    def l1_data_instances(self) -> Optional[int]:
        """Number of L1 data cache instances. :const:`None` if not determined."""
        return optional_int(self._raw.l1_data_instances)

    @property
    def l1_instruction_instances(self) -> Optional[int]:
        """Number of L1 instruction cache instances. :const:`None` if not determined."""
        return optional_int(self._raw.l1_instruction_instances)

    @property
    def l2_instances(self) -> Optional[int]:
        """Number of L2 cache instances. :const:`None` if not determined."""
        return optional_int(self._raw.l2_instances)

    @property
    def l3_instances(self) -> Optional[int]:
        """Number of L3 cache instances. :const:`None` if not determined."""
        return optional_int(self._raw.l3_instances)

    @property
    def l4_instances(self) -> Optional[int]:
        """Number of L4 cache instances. :const:`None` if not determined."""
        return optional_int(self._raw.l4_instances)

    @property
    def cpu_codename(self) -> str:
        """Human-friendly CPU codename."""
        return c_string_to_str(self._raw.cpu_codename)

    @property
    def sse_size(self) -> Optional[int]:
        """SSE execution unit size (64 or 128), :const:`None` if not available."""
        return optional_int(self._raw.sse_size)

    @property
    def detection_hints(self) -> list[enums.CPUHint]:
        """List of CPU detection hints."""
        return [hint for hint in enums.CPUHint if self._raw.detection_hints[hint]]

    @property
    def sgx(self) -> Optional[cpusgx.CPUSGX]:
        """SGX-related features if present."""
        return cpusgx.CPUSGX(self._raw.sgx) if self._raw.sgx.present == 1 else None

    @property
    def affinity_mask(self) -> list[bool]:
        """Boolean mask of the affinity IDs that this processor type is occupying."""
        bit_mask = getattr(self._raw.affinity_mask, "__bits")
        return [bool(bit) for bit in bit_mask]

    @property
    def purpose(self) -> enums.CPUPurpose:
        """The purpose of the CPU type, relevant for hybrid CPUs."""
        return enums.CPUPurpose(self._raw.purpose)

    @property
    def hypervisor(self) -> Optional[enums.HypervisorVendor]:
        """The hypervisor vendor or :const:`None` if not detected."""
        hypervisor = lib.cpuid_get_hypervisor(ffi.NULL, self._raw)
        return enums.HypervisorVendor(hypervisor) if hypervisor != 0 else None
