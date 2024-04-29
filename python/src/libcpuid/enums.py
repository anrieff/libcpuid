"""
Enumeration classes serving as a database of CPU architectures, features,...
"""

from enum import IntEnum, auto


class CPUArchitecture(IntEnum):
    """CPU architecture."""

    UNKNOWN = -1
    X86 = 0
    ARM = auto()


class CPUVendor(IntEnum):
    """CPU vendor."""

    UNKNOWN = -1
    INTEL = 0
    AMD = auto()
    CYRIX = auto()
    NEXGEN = auto()
    TRANSMETA = auto()
    UMC = auto()
    CENTAUR = auto()
    RISE = auto()
    SIS = auto()
    NSC = auto()
    HYGON = auto()


class CPUPurpose(IntEnum):
    """CPU purpose."""

    GENERAL = 0
    PERFORMANCE = auto()
    EFFICIENCY = auto()
    LP_EFFICIENCY = auto()


class CPUFeature(IntEnum):
    """CPU feature identifiers."""

    FPU = 0
    VME = auto()
    DE = auto()
    PSE = auto()
    TSC = auto()
    MSR = auto()
    PAE = auto()
    MCE = auto()
    CX8 = auto()
    APIC = auto()
    MTRR = auto()
    SEP = auto()
    PGE = auto()
    MCA = auto()
    CMOV = auto()
    PAT = auto()
    PSE36 = auto()
    PN = auto()
    CLFLUSH = auto()
    DTS = auto()
    ACPI = auto()
    MMX = auto()
    FXSR = auto()
    SSE = auto()
    SSE2 = auto()
    SS = auto()
    HT = auto()
    TM = auto()
    IA64 = auto()
    PBE = auto()
    PNI = auto()
    PCLMUL = auto()
    DTSS64 = auto()
    MONITOR = auto()
    DS_CPL = auto()
    VMX = auto()
    SMX = auto()
    EST = auto()
    TM2 = auto()
    SSSE3 = auto()
    CID = auto()
    CX16 = auto()
    XTPR = auto()
    PDCM = auto()
    DCA = auto()
    SSE4_1 = auto()
    SSE4_2 = auto()
    SYSCALL = auto()
    XD = auto()
    MOVBE = auto()
    POPCNT = auto()
    AES = auto()
    XSAVE = auto()
    OSXSAVE = auto()
    AVX = auto()
    MMXEXT = auto()
    _3DNOW = auto()
    _3DNOWEXT = auto()
    NX = auto()
    FXSR_OPT = auto()
    RDTSCP = auto()
    LM = auto()
    LAHF_LM = auto()
    CMP_LEGACY = auto()
    SVM = auto()
    ABM = auto()
    MISALIGNSSE = auto()
    SSE4A = auto()
    _3DNOWPREFETCH = auto()
    OSVW = auto()
    IBS = auto()
    SSE5 = auto()
    SKINIT = auto()
    WDT = auto()
    TS = auto()
    FID = auto()
    VID = auto()
    TTP = auto()
    TM_AMD = auto()
    STC = auto()
    _100MHZSTEPS = auto()
    HWPSTATE = auto()
    CONSTANT_TSC = auto()
    XOP = auto()
    FMA3 = auto()
    FMA4 = auto()
    TBM = auto()
    F16C = auto()
    RDRAND = auto()
    X2APIC = auto()
    CPB = auto()
    APERFMPERF = auto()
    PFI = auto()
    PA = auto()
    AVX2 = auto()
    BMI1 = auto()
    BMI2 = auto()
    HLE = auto()
    RTM = auto()
    AVX512F = auto()
    AVX512DQ = auto()
    AVX512PF = auto()
    AVX512ER = auto()
    AVX512CD = auto()
    SHA_NI = auto()
    AVX512BW = auto()
    AVX512VL = auto()
    SGX = auto()
    RDSEED = auto()
    ADX = auto()
    AVX512VNNI = auto()
    AVX512VBMI = auto()
    AVX512VBMI2 = auto()
    HYPERVISOR = auto()


class HypervisorVendor(IntEnum):
    """Hypervisor vendor."""

    UNKNOWN = -1
    BHYVE = auto()
    HYPERV = auto()
    KVM = auto()
    PARALLELS = auto()
    QEMU = auto()
    VIRTUALBOX = auto()
    VMWARE = auto()
    XEN = auto()


class CPUSGXFeature(IntEnum):
    """CPU SGX feature identifiers."""

    INTEL_SGX1 = 0
    INTEL_SGX2 = auto()


class CPUHint(IntEnum):
    """CPU detection hints."""

    SSE_SIZE_AUTH = 0
