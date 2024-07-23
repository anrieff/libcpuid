"""
Utility functions for building the FFI.
"""

import subprocess
import os
import re
import tempfile


class FFIBuildException(Exception):
    """Generic exception for errors occuring during the CFFI build."""


def get_include_flags():
    """
    Obtains libcpuid include flags via pkg-config.
    """
    try:
        cflags = (
            subprocess.check_output(["pkg-config", "libcpuid", "--cflags-only-I"])
            .decode()
            .strip()
            .split()
        )
        return cflags
    except subprocess.CalledProcessError as e:
        if e.returncode == 127:
            raise FFIBuildException(
                "The pkg-config command is necessary to build python-libcpuid."
            ) from e
        if e.returncode == 1:
            raise FFIBuildException(
                "The libcpuid C library (devel) was not found."
            ) from e
        raise FFIBuildException("Error looking for the libcpuid library") from e


def find_header_file(include_flags):
    """
    Obtains main libcpuid header file location from include flags.
    """
    header_path = None  # pylint: disable=invalid-name
    for cflag in include_flags:
        header_candidate = os.path.join(cflag[2:], "libcpuid.h")
        if os.path.isfile(header_candidate):
            header_path = header_candidate
            break
    if header_path is None:
        raise FFIBuildException("Could not find header file of the libcpuid library.")
    return header_path


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
    c_program_path = os.path.join(tmp_dir, "sizeof.c")
    executable_path = os.path.join(tmp_dir, "sizeof")

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
