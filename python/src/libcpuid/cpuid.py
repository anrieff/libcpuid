"""
Classes providing CPU information after identification.
"""

from ctypes import byref, CDLL
from typing import Optional
from . import types, enums


class CPUSGX:
    """
    Holds the SGX-related information of a CPU.

    Obtainable as :attr:`CPUID.sgx`:

    >>> from libcpuid import LibCPUID
    >>> lib = LibCPUID()
    >>> cpuid = lib.cpu_identify()
    >>> sgx = cpuid.sgx
    >>> if sgx is not None:
    >>>     print(sgx.max_enclave_32bit)
    """

    def __init__(self, raw: types.CPUSGX):
        self._raw = raw

    @property
    def max_enclave_32bit(self) -> int:
        """The maximum enclave size in 32-bit mode."""
        return self._raw.max_enclave_32bit

    @property
    def max_enclave_64bit(self) -> int:
        """The maximum enclave size in 64-bit mode."""
        return self._raw.max_enclave_64bit

    def has_feature(self, feature: enums.CPUSGXFeature) -> bool:
        """Checks if the CPU has a specific SGX feature."""
        return self._raw.flags[feature] == 1

    @property
    def num_epc_sections(self) -> int:
        """The number of Enclave Page Cache (EPC) sections."""
        return self._raw.num_epc_sections

    @property
    def misc_select(self) -> bytes:
        """
        Bit vector of the supported extended features that can be written
        to the MISC region of the SSA (Save State Area)
        """
        return self._raw.misc_select.to_bytes(4, byteorder="big")

    @property
    def secs_attributes(self) -> bytes:
        """
        Bit vector of the attributes that can be set to SECS.ATTRIBUTES
        via ECREATE.
        """
        return self._raw.secs_attributes.to_bytes(8, byteorder="big")

    @property
    def secs_xfrm(self) -> bytes:
        """
        Bit vector of the attributes that can be set in the XSAVE feature
        request mask.
        """
        return self._raw.secs_xfrm.to_bytes(8, byteorder="big")


class CPUID:  # pylint: disable=too-many-public-methods
    """
    Holds all acquirable information about a CPU.

    Use :func:`cpu_identify` to create an instance of this class:

    >>> from libcpuid import LibCPUID
    >>> lib = LibCPUID()
    >>> cpuid = lib.cpu_identify()
    >>> print(cpuid.architecture)
    """

    def __init__(self, raw: types.CPUID, dll: CDLL):
        self._raw = raw
        self._dll = dll

    @staticmethod
    def _optional_int(value: int) -> Optional[int]:
        """Returns the given value if it is not -1, otherwise None."""
        return value if value != -1 else None

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
        return self._raw.vendor_str.decode()

    @property
    def brand_str(self) -> str:
        """The CPU brand string, e.g., :const:`'Intel(R) Xeon(TM) CPU 2.40GHz'`."""
        return self._raw.brand_str.decode()

    @property
    def vendor(self) -> enums.CPUVendor:
        """The CPU vendor."""
        try:
            return enums.CPUVendor(self._raw.vendor)
        except ValueError:
            return enums.CPUVendor.UNKNOWN

    def has_feature(self, feature: enums.CPUFeature) -> bool:
        """Checks if the CPU has a specific feature."""
        return self._raw.flags[feature] == 1

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
        return self._optional_int(self._raw.l1_data_cache)

    @property
    def l1_instruction_cache(self) -> Optional[int]:
        """L1 instruction cache size in KB. :const:`None` if not determined."""
        return self._optional_int(self._raw.l1_instruction_cache)

    @property
    def l2_cache(self) -> Optional[int]:
        """L2 cache size in KB. :const:`None` if not determined."""
        return self._optional_int(self._raw.l2_cache)

    @property
    def l3_cache(self) -> Optional[int]:
        """L3 cache size in KB. :const:`None` if not determined."""
        return self._optional_int(self._raw.l3_cache)

    @property
    def l4_cache(self) -> Optional[int]:
        """L4 cache size in KB. :const:`None` if not determined."""
        return self._optional_int(self._raw.l4_cache)

    @property
    def l1_data_assoc(self) -> Optional[int]:
        """L1 data cache associativity. :const:`None` if not determined."""
        return self._optional_int(self._raw.l1_data_assoc)

    @property
    def l1_instruction_assoc(self) -> Optional[int]:
        """L1 instruction cache associativity. :const:`None` if not determined."""
        return self._optional_int(self._raw.l1_instruction_assoc)

    @property
    def l2_assoc(self) -> Optional[int]:
        """L2 cache associativity. :const:`None` if not determined."""
        return self._optional_int(self._raw.l2_assoc)

    @property
    def l3_assoc(self) -> Optional[int]:
        """L3 cache associativity. :const:`None` if not determined."""
        return self._optional_int(self._raw.l3_assoc)

    @property
    def l4_assoc(self) -> Optional[int]:
        """L4 cache associativity. :const:`None` if not determined."""
        return self._optional_int(self._raw.l4_assoc)

    @property
    def l1_data_cacheline(self) -> Optional[int]:
        """L1 data cache line size. :const:`None` if not determined."""
        return self._optional_int(self._raw.l1_data_cacheline)

    @property
    def l1_instruction_cacheline(self) -> Optional[int]:
        """L1 instruction cache line size. :const:`None` if not determined."""
        return self._optional_int(self._raw.l1_instruction_cacheline)

    @property
    def l2_cacheline(self) -> Optional[int]:
        """L2 cache line size. :const:`None` if not determined."""
        return self._optional_int(self._raw.l2_cacheline)

    @property
    def l3_cacheline(self) -> Optional[int]:
        """L3 cache line size. :const:`None` if not determined."""
        return self._optional_int(self._raw.l3_cacheline)

    @property
    def l4_cacheline(self) -> Optional[int]:
        """L4 cache line size. :const:`None` if not determined."""
        return self._optional_int(self._raw.l4_cacheline)

    @property
    def l1_data_instances(self) -> Optional[int]:
        """Number of L1 data cache instances. :const:`None` if not determined."""
        return self._optional_int(self._raw.l1_data_instances)

    @property
    def l1_instruction_instances(self) -> Optional[int]:
        """Number of L1 instruction cache instances. :const:`None` if not determined."""
        return self._optional_int(self._raw.l1_instruction_instances)

    @property
    def l2_instances(self) -> Optional[int]:
        """Number of L2 cache instances. :const:`None` if not determined."""
        return self._optional_int(self._raw.l2_instances)

    @property
    def l3_instances(self) -> Optional[int]:
        """Number of L3 cache instances. :const:`None` if not determined."""
        return self._optional_int(self._raw.l3_instances)

    @property
    def l4_instances(self) -> Optional[int]:
        """Number of L4 cache instances. :const:`None` if not determined."""
        return self._optional_int(self._raw.l4_instances)

    @property
    def cpu_codename(self) -> str:
        """Human-friendly CPU codename."""
        return self._raw.cpu_codename.decode()

    @property
    def sse_size(self) -> Optional[int]:
        """SSE execution unit size (64 or 128), :const:`None` if not available."""
        return self._optional_int(self._raw.sse_size)

    def has_detection_hint(self, hint: enums.CPUHint) -> bool:
        """Checks if a CPU detection hint is present."""
        return self._raw.detection_hints[hint] == 1

    @property
    def sgx(self) -> Optional[CPUSGX]:
        """SGX-related features if present."""
        return CPUSGX(self._raw.sgx) if self._raw.sgx.present == 1 else None

    @property
    def affinity_mask(self) -> bytes:
        """Bitmask of the affinity IDs that this processor type is occupying."""
        return b"".join([byte.to_bytes(1) for byte in self._raw.affinity_mask.bits])

    @property
    def purpose(self) -> enums.CPUPurpose:
        """The purpose of the CPU type, relevant for hybrid CPUs."""
        return enums.CPUPurpose(self._raw.purpose)

    @property
    def hypervisor(self) -> Optional[enums.HypervisorVendor]:
        """The hypervisor vendor or :const:`None` if not detected."""
        hypervisor = self._dll.cpuid_get_hypervisor(None, byref(self._raw))
        return enums.HypervisorVendor(hypervisor) if hypervisor != 0 else None
