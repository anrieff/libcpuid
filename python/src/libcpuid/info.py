"""
The main module, providing a class that holds CPU information.
"""

from typing import Optional
from libcpuid import enums
from libcpuid.sgx import SGX
from libcpuid.raw import CPURawData, CPURawDataArray
from libcpuid.errors import CLibraryError
from libcpuid._utils import c_string_to_str, optional_int
from libcpuid._libcpuid_cffi import (  # pylint: disable=no-name-in-module, import-error
    lib,
    ffi,
)


class CPUInfo:  # pylint: disable=too-many-public-methods
    """
    The main class, holding information about a CPU
    in human-friendly format.
    """

    def __init__(self, c_cpu_id):
        self._c_cpu_id = c_cpu_id
        self._features = None
        self._detection_hints = None

    @classmethod
    def from_c(cls, c_cpu_id):
        """Creates a :class:`CPUInfo` instance from the corresponding C structure."""
        cpu_info = cls(c_cpu_id)
        if cpu_info.architecture == enums.CPUArchitecture.X86:
            return X86Info(c_cpu_id)
        if cpu_info.architecture == enums.CPUArchitecture.ARM:
            return ARMInfo(c_cpu_id)
        return cpu_info

    @classmethod
    def from_current_cpu(cls):
        """Creates a :class:`CPUInfo` instance by identifying the current CPU."""
        c_cpu_id = ffi.new("struct cpu_id_t *")
        if lib.cpu_identify(ffi.NULL, c_cpu_id) != 0:
            raise CLibraryError
        return cls.from_c(c_cpu_id)

    @classmethod
    def from_purpose(cls, purpose: enums.CPUPurpose):
        """Creates a :class:`CPUInfo` instance by identifying a CPU with the requested purpose."""
        c_cpu_id = ffi.new("struct cpu_id_t *")
        if lib.cpu_request_core_type(purpose, ffi.NULL, c_cpu_id) != 0:
            raise CLibraryError
        return CPUInfo.from_c(c_cpu_id)

    @classmethod
    def from_raw(cls, raw_data: CPURawData):
        """Creates a :class:`CPUInfo` instance from an instance of :class:`CPURawData`"""
        c_cpu_id = ffi.new("struct cpu_id_t *")
        if lib.cpu_identify(raw_data.c_cpu_raw_data, c_cpu_id) != 0:
            raise CLibraryError
        return CPUInfo.from_c(c_cpu_id)

    @classmethod
    def from_raw_array_and_purpose(
        cls, raw_data_array: CPURawDataArray, purpose: enums.CPUPurpose
    ):
        """Identifies a CPU with a given purpose, returning a :class:`CPUInfo` instance."""
        c_cpu_id = ffi.new("struct cpu_id_t *")
        if (
            lib.cpu_request_core_type(
                purpose, raw_data_array.c_cpu_raw_data_array, c_cpu_id
            )
            != 0
        ):
            raise CLibraryError
        return CPUInfo.from_c(c_cpu_id)

    @property
    def architecture(self) -> enums.CPUArchitecture:
        """The CPU Architecture."""
        try:
            return enums.CPUArchitecture(self._c_cpu_id.architecture)
        except ValueError:
            return enums.CPUArchitecture.UNKNOWN

    @property
    def feature_level(self) -> enums.CPUFeatureLevel:
        """The CPU feature level."""
        try:
            return enums.CPUFeatureLevel(self._c_cpu_id.feature_level)
        except ValueError:
            return enums.CPUFeatureLevel.UNKNOWN

    @property
    def vendor_str(self) -> str:
        """The CPU vendor string, e.g., :const:`'GenuineIntel'`."""
        return c_string_to_str(self._c_cpu_id.vendor_str)

    @property
    def brand_str(self) -> str:
        """The CPU brand string, e.g., :const:`'Intel(R) Xeon(TM) CPU 2.40GHz'`."""
        return c_string_to_str(self._c_cpu_id.brand_str)

    @property
    def vendor(self) -> enums.CPUVendor:
        """The CPU vendor."""
        try:
            return enums.CPUVendor(self._c_cpu_id.vendor)
        except ValueError:
            return enums.CPUVendor.UNKNOWN

    @property
    def features(self) -> set[enums.CPUFeature]:
        """The set of supported CPU features."""
        if self._features is None:
            self._features = {
                flag
                for flag in enums.CPUFeature
                if flag >= 0 and self._c_cpu_id.flags[flag]
            }
        return self._features

    @property
    def num_cores(self) -> int:
        """The number of CPU cores."""
        return self._c_cpu_id.num_cores

    @property
    def num_logical_cpus(self) -> int:
        """The number of logical processors."""
        return self._c_cpu_id.num_logical_cpus

    @property
    def total_logical_cpus(self) -> int:
        """The total number of logical processors."""
        return self._c_cpu_id.total_logical_cpus

    @property
    def l1_data_cache(self) -> Optional[int]:
        """L1 data cache size in KB. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l1_data_cache)

    @property
    def l1_instruction_cache(self) -> Optional[int]:
        """L1 instruction cache size in KB. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l1_instruction_cache)

    @property
    def l2_cache(self) -> Optional[int]:
        """L2 cache size in KB. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l2_cache)

    @property
    def l3_cache(self) -> Optional[int]:
        """L3 cache size in KB. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l3_cache)

    @property
    def l4_cache(self) -> Optional[int]:
        """L4 cache size in KB. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l4_cache)

    @property
    def l1_data_assoc(self) -> Optional[int]:
        """L1 data cache associativity. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l1_data_assoc)

    @property
    def l1_instruction_assoc(self) -> Optional[int]:
        """L1 instruction cache associativity. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l1_instruction_assoc)

    @property
    def l2_assoc(self) -> Optional[int]:
        """L2 cache associativity. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l2_assoc)

    @property
    def l3_assoc(self) -> Optional[int]:
        """L3 cache associativity. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l3_assoc)

    @property
    def l4_assoc(self) -> Optional[int]:
        """L4 cache associativity. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l4_assoc)

    @property
    def l1_data_cacheline(self) -> Optional[int]:
        """L1 data cache line size. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l1_data_cacheline)

    @property
    def l1_instruction_cacheline(self) -> Optional[int]:
        """L1 instruction cache line size. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l1_instruction_cacheline)

    @property
    def l2_cacheline(self) -> Optional[int]:
        """L2 cache line size. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l2_cacheline)

    @property
    def l3_cacheline(self) -> Optional[int]:
        """L3 cache line size. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l3_cacheline)

    @property
    def l4_cacheline(self) -> Optional[int]:
        """L4 cache line size. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l4_cacheline)

    @property
    def l1_data_instances(self) -> Optional[int]:
        """Number of L1 data cache instances. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l1_data_instances)

    @property
    def l1_instruction_instances(self) -> Optional[int]:
        """Number of L1 instruction cache instances. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l1_instruction_instances)

    @property
    def l2_instances(self) -> Optional[int]:
        """Number of L2 cache instances. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l2_instances)

    @property
    def l3_instances(self) -> Optional[int]:
        """Number of L3 cache instances. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l3_instances)

    @property
    def l4_instances(self) -> Optional[int]:
        """Number of L4 cache instances. :const:`None` if not determined."""
        return optional_int(self._c_cpu_id.l4_instances)

    @property
    def cpu_codename(self) -> str:
        """A human-friendly CPU codename."""
        return c_string_to_str(self._c_cpu_id.cpu_codename)

    @property
    def detection_hints(self) -> set[enums.CPUHint]:
        """A set of miscellaneous detection hints."""
        if self._detection_hints is None:
            self._detection_hints = {
                hint
                for hint in enums.CPUHint
                if hint >= 0 and self._c_cpu_id.detection_hints[hint]
            }
        return self._detection_hints

    @property
    def affinity_mask(self) -> bytes:
        """A bit mask of the affinity IDs that this processor type is occupying."""
        bit_mask = getattr(self._c_cpu_id.affinity_mask, "__bits")
        return b"".join(reversed([byte.to_bytes(1) for byte in bit_mask])).lstrip(
            b"\x00"
        )

    @property
    def purpose(self) -> enums.CPUPurpose:
        """The purpose of the CPU type, relevant for hybrid CPUs."""
        return enums.CPUPurpose(self._c_cpu_id.purpose)

    @property
    def hypervisor(self) -> Optional[enums.HypervisorVendor]:
        """The hypervisor vendor or :const:`None` if not detected."""
        hypervisor = lib.cpuid_get_hypervisor(ffi.NULL, self._c_cpu_id)
        return enums.HypervisorVendor(hypervisor) if hypervisor != 0 else None


class X86Info(CPUInfo):
    """
    The :class:`CPUInfo` child class for x86 CPUs.
    """

    def __init__(self, c_cpu_id):
        super().__init__(c_cpu_id)
        self._c_cpu_id = c_cpu_id

    @property
    def family(self) -> int:
        """The CPU family number."""
        return self._c_cpu_id.family

    @property
    def model(self) -> int:
        """The CPU model number."""
        return self._c_cpu_id.model

    @property
    def stepping(self) -> int:
        """The CPU stepping."""
        return self._c_cpu_id.stepping

    @property
    def ext_family(self) -> int:
        """The CPU display family number."""
        return self._c_cpu_id.ext_family

    @property
    def ext_model(self) -> int:
        """The CPU display model number."""
        return self._c_cpu_id.ext_model

    @property
    def sse_size(self) -> Optional[int]:
        """SSE execution unit size (64 or 128), :const:`None` if not available."""
        return optional_int(self._c_cpu_id.sse_size)

    @property
    def sgx(self) -> Optional[SGX]:
        """SGX-related features if present, otherwise :const:`None`."""
        return SGX(self._c_cpu_id.sgx) if self._c_cpu_id.sgx.present == 1 else None


class ARMInfo(CPUInfo):
    """
    The :class:`CPUInfo` child class for ARM CPUs.
    """

    def __init__(self, c_cpu_id):
        super().__init__(c_cpu_id)
        self._c_cpu_id = c_cpu_id

    @property
    def implementer(self) -> int:
        """The CPU implementer code."""
        return self._c_cpu_id.implementer

    @property
    def variant(self) -> int:
        """The CPU variant number."""
        return self._c_cpu_id.variant

    @property
    def part_num(self) -> int:
        """The CPU primary part number."""
        return self._c_cpu_id.part_num

    @property
    def revision(self) -> int:
        """The CPU revision number."""
        return self._c_cpu_id.revision


class SystemInfo:
    """
    Class for holding structured :class:`CPUInfo` information about multiple CPUs.
    Instances of this class can be indexed like lists, each item (of type :class:`CPUInfo`)
    corresponds to a different :class:`CPUPurpose`.
    """

    def __init__(self, c_system_id, cpu_info_list: list[CPUInfo]):
        self._c_system_id = c_system_id
        self._cpu_info_list = cpu_info_list

    def __del__(self):
        lib.cpuid_free_system_id(self._c_system_id)

    def __getitem__(self, index: int) -> CPUInfo:
        return self._cpu_info_list[index]

    def __len__(self) -> int:
        return len(self._cpu_info_list)

    @classmethod
    def from_c(cls, c_system_id):
        """Create a :class:`SystemInfo` instance from the corresponding C structure."""
        cpu_id_list = []
        for c_cpu_id in c_system_id.cpu_types[0 : c_system_id.num_cpu_types]:
            new_c_cpu_id = ffi.new("struct cpu_id_t *")
            ffi.memmove(
                new_c_cpu_id, ffi.addressof(c_cpu_id), ffi.sizeof("struct cpu_id_t")
            )
            cpu_id_list.append(CPUInfo.from_c(new_c_cpu_id))
        return cls(c_system_id, cpu_id_list)

    @classmethod
    def from_all_cpus(cls):
        """Create a :class:`SystemInfo` instance by indentifying all CPUs."""
        c_system_id = ffi.new("struct system_id_t *")
        if lib.cpu_identify_all(ffi.NULL, c_system_id) != 0:
            raise CLibraryError
        return cls.from_c(c_system_id)

    @classmethod
    def from_raw_array(cls, raw_data_array: CPURawDataArray):
        """Create a :class:`SystemInfo` instance from an instance of :class:`CPURawDataArray`"""
        c_system_id = ffi.new("struct system_id_t *")
        if lib.cpu_identify_all(raw_data_array.c_cpu_raw_data_array, c_system_id) != 0:
            raise CLibraryError
        return SystemInfo.from_c(c_system_id)

    @property
    def l1_data_total_instances(self) -> Optional[int]:
        """The number of total L1 data cache instances. :const:`None` if not determined."""
        return optional_int(self._c_system_id.l1_data_total_instances)

    @property
    def l1_instruction_total_instances(self) -> Optional[int]:
        """The number of total L1 instruction cache instances. :const:`None` if not determined."""
        return optional_int(self._c_system_id.l1_instruction_total_instances)

    @property
    def l2_total_instances(self) -> Optional[int]:
        """The number of total L2 cache instances. :const:`None` if not undetermined"""
        return optional_int(self._c_system_id.l2_total_instances)

    @property
    def l3_total_instances(self) -> Optional[int]:
        """The number of total L3 cache instances. :const:`None` if not undetermined"""
        return optional_int(self._c_system_id.l3_total_instances)

    @property
    def l4_total_instances(self) -> Optional[int]:
        """The number of total L4 cache instances. :const:`None` if not undetermined"""
        return optional_int(self._c_system_id.l4_total_instances)
