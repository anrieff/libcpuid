from setuptools import setup

setup(
    cffi_modules=["ffi_build.py:ffibuilder"],
)
