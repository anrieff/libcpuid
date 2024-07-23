"""
Module providing access to enums from the libcpuid library,
e.g., CPU architectures, vendors, or features.

For compatibility with the underlying C library, the values
of all enums are integers. However, calling the :func:`str`
or :func:`print` function on an enum member returns or prints
its string representation.
"""

from enum import IntEnum
from libcpuid._utils import c_string_to_str, get_enum_options
from libcpuid._libcpuid_cffi import (  # pylint: disable=no-name-in-module, import-error
    lib,
)


CPUArchitecture = IntEnum("CPUArchitecture", get_enum_options("ARCHITECTURE_"))
CPUArchitecture.__str__ = lambda self: c_string_to_str(lib.cpu_architecture_str(self))
CPUArchitecture.__doc__ = "CPU architectures."

CPUVendor = IntEnum("CPUVendor", get_enum_options("VENDOR_"))
CPUVendor.__str__ = lambda self: self.name
CPUVendor.__doc__ = "CPU vendors."

CPUPurpose = IntEnum("CPUPurpose", get_enum_options("PURPOSE_"))
CPUPurpose.__str__ = lambda self: c_string_to_str(lib.cpu_purpose_str(self))
CPUPurpose.__doc__ = "CPU purposes."

HypervisorVendor = IntEnum(
    "HypervisorVendor", get_enum_options("HYPERVISOR_", ignore=["NONE"])
)
HypervisorVendor.__str__ = lambda self: self.name
HypervisorVendor.__doc__ = (
    "Hypervisor vendors, as guessed from the CPU_FEATURE_HYPERVISOR flag"
)

CPUFeature = IntEnum("CPUFeature", get_enum_options("CPU_FEATURE_"))
CPUFeature.__str__ = lambda self: c_string_to_str(lib.cpu_feature_str(self))
CPUFeature.__doc__ = "CPU feature identifiers (flags)"

CPUFeatureLevel = IntEnum("CPUFeatureLevel", get_enum_options("FEATURE_LEVEL_"))
CPUFeatureLevel.__str__ = lambda self: c_string_to_str(lib.cpu_feature_level_str(self))
CPUFeatureLevel.__doc__ = """
    CPU feature levels, also known as microarchitecture levels (x86) or architecture versions (ARM).
"""

CPUHint = IntEnum("CPUHint", get_enum_options("CPU_HINT_"))
CPUHint.__str__ = lambda self: self.name
CPUHint.__doc__ = "CPU detection hint identifiers."

SGXFeature = IntEnum("SGXFeature", get_enum_options("INTEL_"))
SGXFeature.__str__ = lambda self: self.name
SGXFeature.__doc__ = "SGX feature flags."
