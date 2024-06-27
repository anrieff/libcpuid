"""
Module dealing with SGX-related CPU information.
"""

from libcpuid import enums


class CPUSGX:
    """
    Holds the SGX-related information of a CPU.

    Obtainable as :attr:`libcpuid.cpuid.CPUID.sgx`:

    >>> cpuid = libcpuid.cpu_identify()
    >>> sgx = cpuid.sgx
    >>> if sgx is not None:
    >>>     print(sgx.max_enclave_32bit)
    """

    def __init__(self, raw):
        self._raw = raw

    @property
    def max_enclave_32bit(self) -> int:
        """The maximum enclave size in 32-bit mode."""
        return self._raw.max_enclave_32bit

    @property
    def max_enclave_64bit(self) -> int:
        """The maximum enclave size in 64-bit mode."""
        return self._raw.max_enclave_64bit

    @property
    def flags(self) -> list[enums.SGXFeature]:
        """Returns a list of CPU SGX features"""
        return [flag for flag in enums.SGXFeature if self._raw.flags[flag]]

    @property
    def num_epc_sections(self) -> int:
        """The number of Enclave Page Cache (EPC) sections."""
        return self._raw.num_epc_sections

    @property
    def misc_select(self) -> bytes:
        """
        Bit vector of the supported extended features that can be written
        to the MISC region of the SSA (Save State Area)
        """
        return self._raw.misc_select.to_bytes(4, byteorder="big")

    @property
    def secs_attributes(self) -> bytes:
        """
        Bit vector of the attributes that can be set to SECS.ATTRIBUTES
        via ECREATE.
        """
        return self._raw.secs_attributes.to_bytes(8, byteorder="big")

    @property
    def secs_xfrm(self) -> bytes:
        """
        Bit vector of the attributes that can be set in the XSAVE feature
        request mask.
        """
        return self._raw.secs_xfrm.to_bytes(8, byteorder="big")
