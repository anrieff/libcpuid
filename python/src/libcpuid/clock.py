"""
Module for CPU clock/frequency calculation.
"""

from enum import Enum, auto
from libcpuid.errors import LibcpuidError
from libcpuid._libcpuid_cffi import (  # pylint: disable=no-name-in-module, import-error
    lib,
    ffi,
)


def exec_rdtsc() -> int:
    """
    Exectutes the `rdtsc` (read timestamp counter) instruction
    and returns the value of the counter.
    """
    result = ffi.new("uint64_t *")
    lib.cpu_rdtsc(result)
    return result[0]


class TSC:
    """
    Class for calculating clock frequency using differences
    in time and in the timestamp counter.
    """

    class State(Enum):
        """State of the measurement."""

        NOT_STARTED = auto()
        STARTED = auto()
        STOPPED = auto()

    def __init__(self):
        self._c_cpu_mark = ffi.new("struct cpu_mark_t *")
        self._state = self.State.NOT_STARTED

    @property
    def state(self) -> State:
        """Returns the current state of the measurement."""
        return self._state

    def start(self):
        """Starts the measurement."""
        lib.cpu_tsc_mark(self._c_cpu_mark)
        self._state = self.State.STARTED

    def stop(self):
        """Stops the measurement."""
        if self._state == self.State.NOT_STARTED:
            raise LibcpuidError("The measurement has not been started.")
        if self._state == self.State.STOPPED:
            raise LibcpuidError("The measurement has already been stopped.")
        lib.cpu_tsc_unmark(self._c_cpu_mark)
        self._state = self.State.STOPPED

    def frequency(self) -> int:
        """
        Calculates the CPU clock frequency from the time and TSC
        difference between the calls to :meth:`start` and :meth:`stop`.
        """
        if self._state == self.State.NOT_STARTED:
            raise LibcpuidError("The measurementhas not been started.")
        if self._state == self.State.STARTED:
            raise LibcpuidError("The measurement has not been stopped.")
        result = lib.cpu_clock_by_mark(self._c_cpu_mark)
        if result < 0:
            raise LibcpuidError("Could not compute clock frequency.")
        return result


def frequency_measure(millis: int = 100, quad_check: bool = False) -> int:
    """
    Measures the CPU clock frequency by busy-waiting for a specified
    number of milliseconds and calculating the difference between TSC
    values. With the :const:`quad_check` option on, the function will
    run 4 consecutive measurements and return the average of the two
    most consistent results.
    """
    result = lib.cpu_clock_measure(millis, int(quad_check))
    if result < 0:
        raise LibcpuidError("Could not measure clock frequency.")
    return result


def frequency_by_os() -> int:
    """Returns the CPU clock frequency as reported by the OS."""
    result = lib.cpu_clock_by_os()
    if result < 0:
        raise LibcpuidError("Clock frequency unavailable.")
    return result


def frequency_by_ic(millis: int = 50, runs: int = 4) -> int:
    """
    Measures the CPU clock frequency using instruction counting for
    a specified number of milliseconds and runs.
    """
    result = lib.cpu_clock_by_ic(millis, runs)
    if result < 0:
        raise LibcpuidError("Could not measure clock frequency.")
    return result


def frequency_by_tsc() -> int:
    """Computes the CPU clock **base** frequency based on information from `cpuid`."""
    result = lib.cpu_clock_by_tsc(ffi.NULL)
    if result < 0:
        raise LibcpuidError("Could not compute clock frequency.")
    return result


def frequency() -> int:
    """
    All in one method for getting CPU clock frequency. It tries to use the OS
    and falls back to other methods if the OS does not have the information.
    """
    result = lib.cpu_clock()
    if result < 0:
        raise LibcpuidError("Could not get clock frequency.")
    return result
