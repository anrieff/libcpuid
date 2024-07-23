"""
Module for compiling the C FFI.
"""

import os
import sys
from cffi import FFI

sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from _ffi_build_utils import (  # pylint: disable=import-error, wrong-import-position
    get_include_flags,
    find_header_file,
    preprocess_header,
    eval_sizeofs,
)

include_flags = get_include_flags()
preprocessed_header = preprocess_header(find_header_file(include_flags))
no_sizeof_header = eval_sizeofs(preprocessed_header, include_flags)
ffibuilder = FFI()
ffibuilder.cdef(no_sizeof_header)
ffibuilder.set_source_pkgconfig(
    "libcpuid._libcpuid_cffi", ["libcpuid"], "#include <libcpuid.h>"
)
