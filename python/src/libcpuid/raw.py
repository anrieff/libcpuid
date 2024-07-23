"""
Module providing access to raw CPU data.
"""

from libcpuid.errors import CLibraryError
from libcpuid._libcpuid_cffi import (  # pylint: disable=no-name-in-module, import-error
    lib,
    ffi,
)


class CPURawData:
    """
    Class holding raw data about a single logical CPU.
    """

    def __init__(self, c_cpu_raw_data):
        self._c_cpu_raw_data = c_cpu_raw_data

    @property
    def c_cpu_raw_data(self):
        """Returns the underlying C structure."""
        return self._c_cpu_raw_data

    def serialize(self, filename: str):
        """Exports the raw data into a provided file."""
        lib.cpuid_serialize_raw_data(self._c_cpu_raw_data, filename.encode())

    @classmethod
    def from_current_cpu(cls):
        """Creates a :class:`CPURawData` instance by running `cpuid` on the current CPU."""
        c_cpu_raw_data = ffi.new("struct cpu_raw_data_t *")
        if lib.cpuid_get_raw_data(c_cpu_raw_data) != 0:
            raise CLibraryError
        return cls(c_cpu_raw_data)

    @classmethod
    def from_cpu_core(cls, logical_cpu: int):
        """Creates a :class:`CPURawData` instance by running `cpuid` on the specified CPU."""
        c_cpu_raw_data = ffi.new("struct cpu_raw_data_t *")
        if lib.cpuid_get_raw_data_core(c_cpu_raw_data, logical_cpu) != 0:
            raise CLibraryError
        return cls(c_cpu_raw_data)

    @classmethod
    def from_file(cls, filename: str):
        """Creates a :class:`CPURawData` instance by parsing the data from a provided file."""
        c_cpu_raw_data = ffi.new("struct cpu_raw_data_t *")
        if lib.cpuid_deserialize_raw_data(c_cpu_raw_data, filename.encode()) != 0:
            raise CLibraryError
        return cls(c_cpu_raw_data)


class CPURawDataArray:
    """
    Class holding raw data about multiple CPUs.
    Instances of this class can be indexed like lists. each item
    (of type :class:`CPURawData`) holds data about one **logical** CPU.
    """

    def __init__(self, c_cpu_raw_data_array):
        self._c_cpu_raw_data_array = c_cpu_raw_data_array
        self._cpu_raw_data_list = [
            CPURawData(c_cpu_raw_data)
            for c_cpu_raw_data in self._c_cpu_raw_data_array.raw[
                0 : self._c_cpu_raw_data_array.num_raw
            ]
        ]

    def __del__(self):
        lib.cpuid_free_raw_data_array(self._c_cpu_raw_data_array)

    def __getitem__(self, index: int) -> CPURawData:
        return self._cpu_raw_data_list[index]

    def __len__(self) -> int:
        return len(self._cpu_raw_data_list)

    @property
    def c_cpu_raw_data_array(self):
        """Returns the underlying C structure."""
        return self._c_cpu_raw_data_array

    def serialize(self, filename: str):
        """Exports the raw data array into a provided file."""
        lib.cpuid_serialize_all_raw_data(self._c_cpu_raw_data_array, filename.encode())

    @classmethod
    def from_all_cpus(cls):
        """Creates a :class:`CPURawDataArray` instance by running `cpuid` on all CPUs."""
        c_cpu_raw_data_array = ffi.new("struct cpu_raw_data_array_t *")
        if lib.cpuid_get_all_raw_data(c_cpu_raw_data_array) != 0:
            raise CLibraryError
        return cls(c_cpu_raw_data_array)

    @classmethod
    def from_file(cls, filename: str):
        """Creates a :class:`CPURawDataArray` instance by parsing the data from a provided file."""
        c_cpu_raw_data_array = ffi.new("struct cpu_raw_data_array_t *")
        if (
            lib.cpuid_deserialize_all_raw_data(c_cpu_raw_data_array, filename.encode())
            != 0
        ):
            raise CLibraryError
        return cls(c_cpu_raw_data_array)
