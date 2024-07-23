"""
The libcpuid package defines Python bindings to the
libcpuid C library, which provides CPU identification.
"""

from libcpuid._libcpuid_cffi import (  # pylint: disable=no-name-in-module, import-error
    ffi,
    lib,
)
from libcpuid import enums
from libcpuid.errors import CLibraryError
from libcpuid._utils import c_string_to_str


def version() -> str:
    """Returns the version of the libcpuid library."""
    return c_string_to_str(lib.cpuid_lib_version())


def cpuid_present() -> bool:
    """Checks if the `cpuid` instruction is supported."""
    return bool(lib.cpuid_present())


def get_total_cpus() -> int:
    """Returns the total number of logical CPU threads."""
    return lib.cpuid_get_total_cpus()


def get_vendor() -> enums.CPUVendor:
    """Returns the CPU vendor of the current CPU."""
    try:
        return enums.CPUVendor(lib.cpuid_get_vendor())
    except ValueError:
        return enums.CPUVendor.UNKNOWN


def get_cpu_list(vendor: enums.CPUVendor) -> list[str]:
    """Gets a list of CPU :meth:`codenames <info.CPUInfo.cpu_codename>` for a specific vendor."""
    c_cpu_list = ffi.new("struct cpu_list_t *")
    lib.cpuid_get_cpu_list(vendor, c_cpu_list)
    if c_cpu_list.num_entries == 0:
        raise CLibraryError
    cpu_list = [
        c_string_to_str(name) for name in c_cpu_list.names[0 : c_cpu_list.num_entries]
    ]
    lib.cpuid_free_cpu_list(c_cpu_list)
    return cpu_list


def exec_cpuid(
    eax: bytes,
    ebx: bytes = 4 * b"\x00",
    ecx: bytes = 4 * b"\x00",
    edx: bytes = 4 * b"\x00",
) -> tuple[bytes, bytes, bytes, bytes]:
    """Executes the `cpuid` instruction with the given content of input registers."""
    regs = ffi.new(
        "uint32_t[4]",
        [
            int.from_bytes(eax),
            int.from_bytes(ebx),
            int.from_bytes(ecx),
            int.from_bytes(edx),
        ],
    )
    lib.cpu_exec_cpuid_ext(regs)
    return tuple(reg.to_bytes(4) for reg in regs)
