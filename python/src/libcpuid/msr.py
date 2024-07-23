"""
Module providing access to information from model-specific registers.
"""

from typing import Optional
from libcpuid.errors import CLibraryError
from libcpuid._utils import get_enum_options
from libcpuid._libcpuid_cffi import (  # pylint: disable=no-name-in-module, import-error
    lib,
    ffi,
)


class MSR:
    """
    Class for reading information from model-specific registers (MSR).

    Since `rdmsr` is a privileged instruction, you need ring 0 access to
    instantiate this class.

    To see the documentation of properties, look for :const:`cpu_msrinfo_request_t`
    in :const:`libcpuid.h`, the main header file of the underlying C library.
    """

    def __init__(self, core: int = 0):
        self._handle = lib.cpu_msr_driver_open_core(core)
        if self._handle == ffi.NULL:
            raise CLibraryError

    def __del__(self):
        if lib.cpu_msr_driver_close(self._handle) != 0:
            raise CLibraryError

    def serialize_raw_data(self, filename: str):
        """Writes raw MSR data to a file."""
        if lib.msr_serialize_raw_data(self._handle, filename.encode()) != 0:
            raise CLibraryError

    def read_register(self, msr_index: int) -> bytes:
        """Returns the content of a model-specific register with the given index."""
        result = ffi.new("uint64_t *")
        if lib.cpu_rdmsr(self._handle, msr_index, result) != 0:
            raise CLibraryError
        return result[0].to_bytes(8)


def _get_msrinfo_request(enum_value):
    def _msrinfo_request(self) -> Optional[int]:
        """May return :const:`None` if not available."""
        result = lib.cpu_msrinfo(
            self._handle, enum_value  # pylint: disable=protected-access
        )
        return None if result == 0x3FFFFFFF else result

    return _msrinfo_request


# This adds properties to the MSR class, copying the possible
# inputs to the cpu_msrinfo function of the C library, such as
# mperf, aperf, min_multiplier, temperature, or voltage.
# See cpu_msr_info_request_t in libcpuid.h for details.
for name, value in get_enum_options("INFO_", ignore=["BCLK"]).items():
    setattr(MSR, name.lower(), property(_get_msrinfo_request(value)))
