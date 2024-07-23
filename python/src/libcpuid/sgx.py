"""
Module dealing with SGX-related CPU information.
"""

from libcpuid import enums
from libcpuid._libcpuid_cffi import (  # pylint: disable=no-name-in-module, import-error
    lib,
    ffi,
)


class EPC:
    """
    Class holding information about a single EPC (Enclave Page Cache) area.
    """

    def __init__(self, c_epc):
        self._c_epc = c_epc

    @property
    def start_addr(self) -> int:
        """Start address."""
        return self._c_epc.start_addr

    @property
    def length(self) -> int:
        """Length of the area."""
        return self._c_epc.length


class SGX:
    """
    Class holding the SGX-related information about a CPU.
    """

    def __init__(self, c_sgx):
        self._c_sgx = c_sgx
        self._features = None

    @property
    def max_enclave_32bit(self) -> int:
        """The maximum enclave size in 32-bit mode."""
        return self._c_sgx.max_enclave_32bit

    @property
    def max_enclave_64bit(self) -> int:
        """The maximum enclave size in 64-bit mode."""
        return self._c_sgx.max_enclave_64bit

    @property
    def features(self) -> set[enums.SGXFeature]:
        """Set of CPU SGX features."""
        if self._features is None:
            self._features = {
                flag
                for flag in enums.SGXFeature
                if flag >= 0 and self._c_sgx.flags[flag]
            }
        return self._features

    @property
    def num_epc_sections(self) -> int:
        """The number of Enclave Page Cache (EPC) sections."""
        return self._c_sgx.num_epc_sections

    @property
    def misc_select(self) -> bytes:
        """
        Bit vector of the supported extended features that can be written
        to the MISC region of the SSA (Save State Area)
        """
        return self._c_sgx.misc_select.to_bytes(4)

    @property
    def secs_attributes(self) -> bytes:
        """
        Bit vector of the attributes that can be set to SECS.ATTRIBUTES
        via ECREATE.
        """
        return self._c_sgx.secs_attributes.to_bytes(8)

    @property
    def secs_xfrm(self) -> bytes:
        """
        Bit vector of the attributes that can be set in the XSAVE feature
        request mask.
        """
        return self._c_sgx.secs_xfrm.to_bytes(8)

    def get_epc(self, index: int) -> EPC:
        """Fetches information about a single EPC area given by `index`."""
        if not 0 <= index < self.num_epc_sections:
            raise IndexError(
                f"Invalid index {index}, the CPU has {self.num_epc_sections} EPC sections."
            )
        return EPC(lib.cpuid_get_epc(index, ffi.NULL))
