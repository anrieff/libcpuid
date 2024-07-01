"""
Module providing access to enums from the libcpuid library,
e.g., CPU architectures, vendors, or features.
"""

from enum import IntEnum
from libcpuid._libcpuid_cffi import (  # pylint: disable=no-name-in-module, import-error
    lib,
)


def _construct_enum(name, prefix):
    enum_options = [x for x in dir(lib) if x.startswith(prefix)]
    enum_dict = {x[len(prefix) :]: getattr(lib, x) for x in enum_options}
    return IntEnum(name, enum_dict)


CPUArchitecture = _construct_enum("CPUArchitecture", "ARCHITECTURE_")
CPUVendor = _construct_enum("CPUVendor", "VENDOR_")
CPUPurpose = _construct_enum("CPUPurpose", "PURPOSE_")
HypervisorVendor = _construct_enum("HypervisorVendor", "HYPERVISOR_")
CPUFeature = _construct_enum("CPUFeature", "CPU_FEATURE_")
CPUHint = _construct_enum("CPUHint", "CPU_HINT_")
SGXFeature = _construct_enum("SGXFeature", "INTEL_")
MSRInfoRequest = _construct_enum("MSRInfoRequest", "INFO_")
