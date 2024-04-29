"""
The libcpuid package defines Python bindings to the
libcpuid C library, which provides CPU identification.
"""

from sys import platform
from ctypes import CDLL, c_bool, c_char_p, POINTER, byref
from . import enums, types, cpuid


class LibCPUIDError(Exception):
    """Raised when an error occurs in the libcpuid library."""


class LibCPUID:
    """The main class wrapping the libcpuid C library."""

    def __init__(self):
        if platform.startswith("win32") or platform.startswith("cygwin"):
            dll_name = "libcpuid.dll"
        elif platform.startswith("linux"):
            dll_name = "libcpuid.so"
        else:
            raise LibCPUIDError(f"Unsupported platform: {platform}")
        try:
            self._dll = CDLL(dll_name)
            self._set_function_prototypes()
        except OSError as ex:
            raise LibCPUIDError("Unable to load the libcpuid library") from ex

    def _check_error(self, return_code: int) -> None:
        """Translates return codes to exceptions."""
        if return_code != 0:
            raise LibCPUIDError(self._dll.cpuid_error().decode())

    def _set_function_prototypes(self):
        """Sets the C function prototypes according to the libcpuid API."""
        # char *cpuid_lib_version(void);
        self._dll.cpuid_lib_version.argtypes = []
        self._dll.cpuid_lib_version.restype = c_char_p

        # bool cpuid_present(void)
        self._dll.cpuid_present.argtypes = []
        self._dll.cpuid_present.restype = c_bool

        # int cpu_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
        self._dll.cpu_identify.argtypes = [
            POINTER(types.CPURawData),
            POINTER(types.CPUID),
        ]
        self._dll.cpu_identify.restype = self._check_error

        # void cpuid_get_cpu_list(cpu_vendor_t vendor, struct cpu_list_t* list)
        self._dll.cpuid_get_cpu_list.argtypes = [
            types.CPUVendor,
            POINTER(types.CPUList),
        ]
        self._dll.cpuid_get_cpu_list.restype = None

        # void cpuid_free_cpu_list(struct cpu_list_t* list);
        self._dll.cpuid_free_cpu_list.argtypes = [POINTER(types.CPUList)]
        self._dll.cpuid_free_cpu_list.restype = None

        # const char* cpuid_error(void);
        self._dll.cpuid_error.argtypes = []
        self._dll.cpuid_error.restype = c_char_p

        # hypervisor_vendor_t
        # cpuid_get_hypervisor(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
        self._dll.cpuid_get_hypervisor.argtypes = [
            POINTER(types.CPURawData),
            POINTER(types.CPUID),
        ]
        self._dll.cpuid_get_hypervisor.restype = types.HypervisorVendor

    def version(self) -> str:
        """Returns the version of the libcpuid library."""
        return self._dll.cpuid_lib_version().decode()

    def cpuid_present(self) -> bool:
        """Checks if the cpuid instruction is supported."""
        return self._dll.cpuid_present()

    def get_total_cpus(self) -> int:
        """Returns the total number of logical CPU threads."""
        return self._dll.cpuid_get_total_cpus()

    def cpu_identify(self) -> cpuid.CPUID:
        """Identifies the CPU and returns a :class:`cpuid.CPUID` instance."""
        cpu_id = types.CPUID()
        self._dll.cpu_identify(None, byref(cpu_id))
        return cpuid.CPUID(cpu_id, self._dll)

    def get_cpu_list(self, vendor: enums.CPUVendor) -> list[str]:
        """Gets a list of CPU :meth:`codenames <cpuid.CPUID.cpu_codename>` for a specific vendor."""
        cpu_list = types.CPUList()
        self._dll.cpuid_get_cpu_list(vendor, byref(cpu_list))
        if cpu_list.num_entries == 0:
            raise LibCPUIDError(self._dll.cpuid_error().decode())
        return [name.decode() for name in cpu_list.names[: cpu_list.num_entries]]
