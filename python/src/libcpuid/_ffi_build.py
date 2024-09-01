"""
Module for compiling the C FFI.
"""

import os
import subprocess
import tempfile
import re
import argparse
from pathlib import Path
from cffi import FFI


class FFIBuildException(Exception):
    """Generic exception for errors occuring during the CFFI build."""


def preprocess_header(header_path):
    """
    Preprocesses the header file (python-cffi only accepts preprocessed C definitions)
    at the given path and returns it as a string.
    """
    try:
        return subprocess.check_output(
            ["gcc", "-U __GNUC__", "-E", header_path]
        ).decode()
    except subprocess.CalledProcessError as e:
        if e.returncode == 127:
            raise FFIBuildException(
                "The gcc compiler is necessary to build python-libcpuid."
            ) from e
        raise FFIBuildException(
            f"Error preprocessing the libcpuid header file: {e.stderr}"
        ) from e


def _get_sizeof_eval_source(sizeof):
    return f"""
#include <libcpuid.h>
#include <stdio.h>

int main() {{
    printf("%ld", {sizeof});
    return 0;
}}
"""


def eval_sizeofs(header, cflags):
    """
    Evaluates each sizeof found in the given C header and replaces all
    occurences of the sizeof with its computed value.
    """
    sizeofs = set(re.findall(r"sizeof\([^\)]*\)", header))
    tmp_dir = tempfile.mkdtemp()
    c_program_path = Path(tmp_dir, "sizeof.c")
    executable_path = Path(tmp_dir, "sizeof")

    for sizeof in sizeofs:
        with open(c_program_path, "w", encoding="UTF-8") as c_program_file:
            c_program_file.write(_get_sizeof_eval_source(sizeof))
        subprocess.check_call(["gcc", c_program_path, *cflags, "-o", executable_path])
        size = subprocess.check_output([executable_path]).decode()
        header = header.replace(sizeof, size)

    os.remove(c_program_path)
    os.remove(executable_path)
    os.rmdir(tmp_dir)
    return header


LIBCPUID_DIR = str(Path(*(Path(os.path.abspath(__file__)).parts[:-4])))
LIBCPUID_INCLUDE_DIR = str(Path(LIBCPUID_DIR, "libcpuid"))
LIBCPUID_LIBRARY_DIR = str(Path(LIBCPUID_DIR, "libcpuid", ".libs"))
LIBCPUID_MAIN_HEADER_FILENAME = "libcpuid.h"
LIBCPUID_MAIN_HEADER_PATH = str(
    Path(LIBCPUID_INCLUDE_DIR, LIBCPUID_MAIN_HEADER_FILENAME)
)
LIBCPUID_LIBRARY_NAME = "cpuid"
PYTHON_SRC_DIR = str(Path(LIBCPUID_DIR, "python", "src"))

PREPROCESSED_HEADER = preprocess_header(LIBCPUID_MAIN_HEADER_PATH)
EVAL_SIZEOF_CFLAGS = [
    f"-I{LIBCPUID_INCLUDE_DIR}",
    f"-L{LIBCPUID_LIBRARY_DIR}",
    f"-l{LIBCPUID_LIBRARY_NAME}",
    f"-Wl,-rpath={LIBCPUID_LIBRARY_DIR}",
]

NO_SIZEOF_HEADER = eval_sizeofs(PREPROCESSED_HEADER, EVAL_SIZEOF_CFLAGS)

ffibuilder = FFI()
ffibuilder.cdef(NO_SIZEOF_HEADER)

set_source_kwargs = {
    "module_name": "libcpuid._libcpuid_cffi",
    "source": f"#include <{LIBCPUID_MAIN_HEADER_FILENAME}>",
    "libraries": [LIBCPUID_LIBRARY_NAME],
    "include_dirs": [LIBCPUID_INCLUDE_DIR],
    "library_dirs": [LIBCPUID_LIBRARY_DIR],
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-r", "--runtime-link", action="store_true")
    args = parser.parse_args()
    if args.runtime_link:
        set_source_kwargs["extra_link_args"] = [f"-Wl,-rpath={LIBCPUID_LIBRARY_DIR}"]

ffibuilder.set_source(**set_source_kwargs)

if __name__ == "__main__":
    ffibuilder.compile(PYTHON_SRC_DIR)
