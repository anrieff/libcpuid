from setuptools import setup

setup(
    cffi_modules=["src/libcpuid/_ffi_build.py:ffibuilder"],
)
