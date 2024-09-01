"""Sanity tests for the libcpuid package."""

import os
import tempfile
import libcpuid
from libcpuid.info import CPUInfo, SystemInfo
from libcpuid.raw import CPURawData, CPURawDataArray
from libcpuid.errors import CLibraryError


def test_cpu_name_in_vendor_list():
    """
    Checks that the current CPU codename appears
    in the list of all CPUs of its vendor.
    """
    try:
        info = CPUInfo.from_current_cpu()
        cpulist = libcpuid.get_cpu_list(info.vendor)
        assert info.cpu_codename in cpulist
    except CLibraryError:
        pass


def test_serialization():
    """
    Checks that CPU data serialization and
    deserialization does not raise any errors.
    """
    with tempfile.TemporaryDirectory() as tmpdirname:
        info_file = os.path.join(tmpdirname, "info.txt")
        raw = CPURawData.from_current_cpu()
        raw.serialize(info_file)
        info = CPUInfo.from_raw(raw)
        info_from_file = CPUInfo.from_raw(CPURawData.from_file(info_file))
        assert info.features == info_from_file.features


def test_array_serialization():
    """
    Checks that CPU array data serialization and
    deserialization does not raise any errors.
    """
    with tempfile.TemporaryDirectory() as tmpdirname:
        info_file = os.path.join(tmpdirname, "info.txt")
        raw_array = CPURawDataArray.from_all_cpus()
        raw_array.serialize(info_file)
        info = SystemInfo.from_raw_array(raw_array)
        info_from_file = SystemInfo.from_raw_array(CPURawDataArray.from_file(info_file))
        assert len(info) == len(info_from_file)
        assert info[0].features == info_from_file[0].features
