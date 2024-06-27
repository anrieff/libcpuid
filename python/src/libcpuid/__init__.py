"""
The libcpuid package defines Python bindings to the
libcpuid C library, which provides CPU identification.
"""

from libcpuid._libcpuid_cffi import (  # pylint: disable=no-name-in-module, import-error
    ffi,
    lib,
)
from libcpuid import cpuid, enums
from libcpuid._utils import c_string_to_str


class LibCPUIDError(Exception):
    """Raised when an error occurs in the libcpuid library."""


def _check_error(return_code: int) -> None:
    """Translates return codes to exceptions."""
    if return_code != 0:
        raise LibCPUIDError(c_string_to_str(lib.cpuid_error()))


def libcpuid_version() -> str:
    """Returns the version of the libcpuid library."""
    return c_string_to_str(lib.cpuid_lib_version())


def cpuid_present() -> bool:
    """Checks if the cpuid instruction is supported."""
    return bool(lib.cpuid_present())


def get_total_cpus() -> int:
    """Returns the total number of logical CPU threads."""
    return lib.cpuid_get_total_cpus()


def cpu_identify() -> cpuid.CPUID:
    """Identifies the CPU and returns a :class:`cpuid.CPUID` instance."""
    raw_cpu_id = ffi.new("struct cpu_id_t *")
    _check_error(lib.cpu_identify(ffi.NULL, raw_cpu_id))
    return cpuid.CPUID(raw_cpu_id)


def get_cpu_list(vendor: enums.CPUVendor) -> list[str]:
    """Gets a list of CPU :meth:`codenames <cpuid.CPUID.cpu_codename>` for a specific vendor."""
    raw_cpu_list = ffi.new("struct cpu_list_t *")
    lib.cpuid_get_cpu_list(vendor, raw_cpu_list)
    if raw_cpu_list.num_entries == 0:
        raise LibCPUIDError(c_string_to_str(lib.cpuid_error()))
    return [
        c_string_to_str(name)
        for name in raw_cpu_list.names[0 : raw_cpu_list.num_entries]
    ]
