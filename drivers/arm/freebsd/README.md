# CPUID FreeBSD kernel module for ARM

This kernel module allows to:
- access AArch32 registers in AArch32 state by using the [`MRC` instruction](https://developer.arm.com/documentation/dui0489/i/arm-and-thumb-instructions/mrc-and-mrc2)
- access both AArch32 and AArch64 registers in AArch64 state by using the [`MRS` instruction](https://developer.arm.com/documentation/dui0473/m/arm-and-thumb-instructions/mrs--system-coprocessor-register-to-arm-register-)

## Build

The module is built during `cmake --build build` and present in the build directory. You can load it by using:
```shell
kldload build/drivers/arm/freebsd/cpuid.ko
```
