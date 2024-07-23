"""
Internal module containing utility functions.
"""

from typing import Optional
from libcpuid._libcpuid_cffi import (  # pylint: disable=no-name-in-module, import-error
    ffi,
    lib,
)


def c_string_to_str(c_string) -> str:
    """Converts an FFI C string to a Python string."""
    return ffi.string(c_string).decode()


def optional_int(value: int) -> Optional[int]:
    """Returns the given integer if it is not -1, otherwise None."""
    return value if value != -1 else None


def get_enum_options(  # pylint: disable=dangerous-default-value
    prefix, ignore=[]
) -> dict[str, int]:
    """Returns a dictionary of C enum option names and their values based on name prefix."""
    enum_options = [x for x in dir(lib) if x.startswith(prefix)]
    return {
        x[len(prefix) :]: getattr(lib, x)
        for x in enum_options
        if x[len(prefix) :] not in ignore
    }
