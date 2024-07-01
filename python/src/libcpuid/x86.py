"""
Module dealing with x86-specific CPU information.
"""

from typing import Optional
from libcpuid import sgx, cpuid
from libcpuid._utils import optional_int


class X86ID(cpuid.CPUID):
    """
    x86 CPU information.
    """

    def __init__(self, raw):
        super().__init__(raw)
        self._raw = raw

    @property
    def family(self) -> int:
        """The CPU family number."""
        return self._raw.family

    @property
    def model(self) -> int:
        """The CPU model number."""
        return self._raw.model

    @property
    def stepping(self) -> int:
        """The CPU stepping."""
        return self._raw.stepping

    @property
    def ext_family(self) -> int:
        """The CPU display family number."""
        return self._raw.ext_family

    @property
    def ext_model(self) -> int:
        """The CPU display model number."""
        return self._raw.ext_model

    @property
    def sse_size(self) -> Optional[int]:
        """SSE execution unit size (64 or 128), :const:`None` if not available."""
        return optional_int(self._raw.sse_size)

    @property
    def sgx(self) -> Optional[sgx.SGX]:
        """SGX-related features if present."""
        return sgx.SGX(self._raw.sgx) if self._raw.sgx.present == 1 else None
