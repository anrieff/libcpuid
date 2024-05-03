"""Sanity tests for the libcpuid package."""

import libcpuid


def test_cpu_name_in_vendor_list():
    """
    Check that the current CPU codename appears
    in the list of all CPUs of its vendor.
    """
    lib = libcpuid.LibCPUID()
    cpuid = lib.cpu_identify()
    cpulist = lib.get_cpu_list(cpuid.vendor)
    assert cpuid.cpu_codename in cpulist
