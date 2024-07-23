"""
Script for compiling the C FFI for the live documentation.
"""

import sys
import os
from cffi import FFI

sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from _ffi_build_utils import (  # pylint: disable=import-error, wrong-import-position
    preprocess_header,
    eval_sizeofs,
)

if __name__ == "__main__":
    header_path = sys.argv[1]
    install_dir = sys.argv[2]
    library_dir = os.path.join(os.getcwd(), install_dir, "lib")
    include_dir = os.path.join(install_dir, "include", "libcpuid")
    ffibuilder = FFI()
    ffibuilder.cdef(eval_sizeofs(preprocess_header(header_path), [f"-I{include_dir}"]))
    ffibuilder.set_source(
        "python.src.libcpuid._libcpuid_cffi",
        "#include <libcpuid.h>",
        libraries=["cpuid"],
        library_dirs=[library_dir],
        include_dirs=[include_dir],
        extra_link_args=[f"-Wl,-rpath={library_dir}"],
    )
    ffibuilder.compile(verbose=True)
