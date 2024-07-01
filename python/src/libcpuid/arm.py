"""
Module dealing with ARM-specific CPU information.
"""

from libcpuid import cpuid


class ARMID(cpuid.CPUID):
    """
    ARM CPU information.
    """

    def __init__(self, raw):
        super().__init__(raw)
        self._raw = raw

    @property
    def implementer(self) -> int:
        """CPU implementer code."""
        return self._raw.implementer

    @property
    def variant(self) -> int:
        """CPU variant number."""
        return self._raw.variant

    @property
    def part_num(self) -> int:
        """CPU primary part number."""
        return self._raw.part_num

    @property
    def revision(self) -> int:
        """CPU revision number."""
        return self._raw.revision
