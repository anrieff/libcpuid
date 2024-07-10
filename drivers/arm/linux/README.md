# CPUID Linux kernel module for ARM

This kernel module allows to:
- access AArch32 registers in AArch32 state by using the [`MRC` instruction](https://developer.arm.com/documentation/dui0489/i/arm-and-thumb-instructions/mrc-and-mrc2)
- access both AArch32 and AArch64 registers in AArch64 state by using the [`MRS` instruction](https://developer.arm.com/documentation/dui0473/m/arm-and-thumb-instructions/mrs--system-coprocessor-register-to-arm-register-)

## Build

### Without DKMS

Building the kernel module by using CMake is useful for libcpuid developers. This method is discouraged for end-users, because you will need to rebuild this module on every kernel updates.

To build the kernel module in the build directory, use CMake with the `-DLIBCPUID_DRIVER_ARM_LINUX_DKMS=OFF` option.  
Then, you can load the module by using:
```shell
# insmod build/drivers/arm/linux/cpuid.ko
```

### With DKMS

Once the project is installed (`cmake --install build`), DKMS files will be present under `/usr/src`.  
Use the `dkms` command to build the module, and replace `X.Y.Z` with libcpuid version:
```shell
# dkms install -m cpuid -v X.Y.Z
# modprobe cpuid
```
