"""
Internal module containing utility functions.
"""

from typing import Optional
from libcpuid._libcpuid_cffi import (  # pylint: disable=no-name-in-module, import-error
    ffi,
)


def c_string_to_str(c_string) -> str:
    """Converts an FFI C string to a Python string."""
    return ffi.string(c_string).decode()


def optional_int(value: int) -> Optional[int]:
    """Returns the given integer if it is not -1, otherwise None."""
    return value if value != -1 else None
