"""
Script for compiling the C FFI for the live documentation.
"""

import subprocess
import sys
import os
from cffi import FFI

if __name__ == "__main__":
    header = sys.argv[1]
    install_dir = sys.argv[2]
    library_dir = os.path.join(os.getcwd(), install_dir, "lib")
    ffibuilder = FFI()
    ffibuilder.cdef(
        subprocess.check_output(["gcc", "-U __GNUC__", "-E", header]).decode()
    )
    ffibuilder.set_source(
        "python.src.libcpuid._libcpuid_cffi",
        "#include <libcpuid.h>",
        libraries=["cpuid"],
        library_dirs=[library_dir],
        include_dirs=[os.path.join(install_dir, "include", "libcpuid")],
        extra_link_args=[f"-Wl,-rpath={library_dir}"],
    )
    ffibuilder.compile(verbose=True)
