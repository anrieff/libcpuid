"""
Module containing custom exceptions of the library.
"""

from libcpuid._utils import c_string_to_str
from libcpuid._libcpuid_cffi import (  # pylint: disable=no-name-in-module, import-error
    lib,
)


class LibcpuidError(Exception):
    """Main exception of the library."""


class CLibraryError(LibcpuidError):
    """
    Raised when an error code is returned by some function
    in the underlying libcpuid C library.
    """

    def __init__(self):
        super().__init__(c_string_to_str(lib.cpuid_error()))
