# libcpuid

libcpuid provides CPU identification. Project website is on SourceForge (https://libcpuid.sourceforge.net).

## Support

### CPU architectures

- x86:
  - 32-bit CPUs (IA-32, also known as i386, i486, i586 and i686)
  - 64-bit CPUs (x86_64, also known as x64, AMD64, and Intel 64)
- ARM (since v0.7.0):
  - 32-bit CPUs (ARM32, also known as AArch32)
  - 64-bit CPUs (ARM64, also known as AArch64)

### Operating systems

libcpuid support varies depending on the features:
- build: code build, generate libraries and binaries
- x86 MSR : access to x86 model-specific registers
- ARM CPUID : access to ARM registers

|Name|Build|x86 MSR|ARM CPUID|
|----|----|-------|---------|
|DragonFly BSD|:heavy_check_mark: ![DragonFly BSD workflow status](https://github.com/anrieff/libcpuid/actions/workflows/build_dragonflybsd.yml/badge.svg)|:grey_question: ([`cpuctl` kernel module](https://man.freebsd.org/cgi/man.cgi?query=cpuctl))|:x:|
|FreeBSD|:heavy_check_mark: ![FreeBSD workflow status](https://github.com/anrieff/libcpuid/actions/workflows/build_freebsd.yml/badge.svg)|:heavy_check_mark: ([`cpuctl` kernel module](https://man.freebsd.org/cgi/man.cgi?query=cpuctl))|:heavy_check_mark:¹ ([`cpuid` out-of-tree kernel module](https://github.com/anrieff/libcpuid/tree/master/drivers/arm/freebsd))|
|Linux|:heavy_check_mark: ![Linux workflow status](https://github.com/anrieff/libcpuid/actions/workflows/build_linux.yml/badge.svg)|:heavy_check_mark: ([`msr` kernel module](https://man7.org/linux/man-pages/man4/msr.4.html))|:heavy_check_mark:¹ ([`cpuid` out-of-tree kernel module](https://github.com/anrieff/libcpuid/tree/master/drivers/arm/linux))|
|macOS|:heavy_check_mark: ![macOS workflow status](https://github.com/anrieff/libcpuid/actions/workflows/build_macos.yml/badge.svg)|:x:|:x:|
|NetBSD|:heavy_check_mark: ![NetBSD BSD workflow status](https://github.com/anrieff/libcpuid/actions/workflows/build_netbsd.yml/badge.svg)|:x:|:x:|
|OpenBSD|:heavy_check_mark: ![OpenBSD BSD workflow status](https://github.com/anrieff/libcpuid/actions/workflows/build_openbsd.yml/badge.svg)|:x:|:x:|
|Windows|:heavy_check_mark: ![Windows workflow status](https://github.com/anrieff/libcpuid/actions/workflows/build_windows.yml/badge.svg)|:heavy_check_mark: ([`msr` driver](https://github.com/anrieff/libcpuid/tree/master/drivers/x86/windows/msr))|:x:|

Legend:
- :heavy_check_mark: Supported
- :grey_question: Likely supported, not heavily tested
- :x: Not supported
- **¹** On AArch32 state, the kernel module is required. On AArch64 state, the kernel module is optional, because the kernel can trap the [`MRS` instruction](https://developer.arm.com/documentation/dui0473/m/arm-and-thumb-instructions/mrs--system-coprocessor-register-to-arm-register-)

## Getting started

You have two ways to get libcpuid:
- build it from sources
- download a pre-compiled binary

### Sources

#### Prerequisites

Using libcpuid requires no dependencies on any of the supported OSes.
Building it requires build tool commands to be available,
which is a matter of installing a few common packages
with related names (e.g. automake, autoconf, libtool, cmake).
It also requires a POSIX-compatible shell. On NetBSD, you may need
to install one (credits to @brucelilly):

1. Install a POSIX-compatible shell such as ksh93
```shell
pkg_add ast-ksh || pkgin in ast-ksh
```
2. Export `CONFIG_SHELL` with correct path if required:
```shell
export CONFIG_SHELL=/usr/pkg/bin/ksh93
```

#### Configuring after checkout

Two build systems are supported, use the one you prefer.

##### By using autotools

Under POSIX systems, where you download the sources, there's no
configure script to run. This is because it isn't a good practice to keep
such scripts in a source control system. To create it, you need to run the
following commands once, after you checkout the libcpuid sources
from GitHub:
```shell
libtoolize
autoreconf --install
```

You need to have `autoconf`, `automake` and `libtool` installed.

After that you can run `./configure` and `make` - this will build
the library.

`make dist` will create a tarball (with "configure" inside) with the
sources.

##### By using CMake

CMake options for libcpuid (use `cmake -LH` to list all options):
- `LIBCPUID_ENABLE_DOCS`: enable building documentation by using Doxyen (**ON** by default)
- `LIBCPUID_ENABLE_TESTS`: enable tests targets, like `test-fast`, `test-old` and `fix-tests` (**OFF** by default)
- `LIBCPUID_BUILD_DEPRECATED`: build support of deprecated attributes (**ON** by default to guarantee backward compatibility)
- `LIBCPUID_BUILD_DRIVERS`: enable building kernel drivers (**ON** by default)
- `LIBCPUID_DRIVER_DEBUG`: enable debug mode flr kernel drivers (**OFF** by default)
- `LIBCPUID_DRIVER_ARM_LINUX_DKMS`: use DKMS for CPUID Linux kernel module for ARM (**ON** by default), switch off to build the kernel module in the `build` directory

Basic example to build and install libcpuid by using CMake:
```shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLIBCPUID_ENABLE_TESTS=ON
cmake --build build
cmake --install build # may need administrative privileges, install under /usr/local by default
```

### Downloads

You can find latest versioned archives [here](https://github.com/anrieff/libcpuid/releases/latest), with binaries for macOS and Windows.

#### Binary packages

Also, libcpuid is available for following systems in official repositories:

* [Arch Linux](https://archlinux.org/packages/extra/x86_64/libcpuid/): `pacman -S libcpuid`
* [Debian (since version 11 "Bullseye")](https://packages.debian.org/source/bullseye/libcpuid): `apt install cpuidtool libcpuid-dev`
* [Fedora (since version 25)](https://src.fedoraproject.org/rpms/libcpuid): `dnf install libcpuid libcpuid-devel`
* [FreeBSD (since version 11)](https://www.freshports.org/sysutils/libcpuid): `pkg install libcpuid`
* [OpenMandriva Lx (since version 4.0 "Nitrogen")](https://github.com/OpenMandrivaAssociation/libcpuid): `dnf install libcpuid-tools libcpuid-devel`
* [openSUSE Leap (since version 15.1)](https://software.opensuse.org/package/libcpuid): `zypper install libcpuid-tools libcpuid-devel`
* [Solus](https://packages.getsol.us/shannon/libc/libcpuid/): `eopkg install libcpuid libcpuid-devel`
* [Ubuntu (since version 20.04 "Focal Fossa")](https://packages.ubuntu.com/source/focal/libcpuid) : `apt install cpuidtool libcpuid-dev`

Below, the full lists of repositories:  
[![Packaging status](https://repology.org/badge/vertical-allrepos/libcpuid.svg?exclude_unsupported=1)](https://repology.org/project/libcpuid/versions)

#### Build tool

* Vcpkg: `vcpkg install cpuid`

## Usage

### As a software developer

For details about the programming API, you might want to take a look at the short [tutorial](https://libcpuid.sourceforge.net/documentation.html), as well as the full [API reference](https://anrieff.github.io/libcpuid).

### As an end-user

You can use the `cpuid_tool` command. Pass the `--help` argument to display usage:
```
Usage: cpuid_tool [options]

Options:
  -h, --help       - Show this help
  --load=<file>    - Load raw CPUID data from file
  --save=<file>    - Acquire raw CPUID data and write it to file
  --report, --all  - Report all decoded CPU info (w/o clock)
  --clock          - in conjunction to --report: print CPU clock as well
  --clock-rdtsc    - same as --clock, but use RDTSC for clock detection
  --cpulist        - list all known CPUs
  --sgx            - list SGX leaf data, if SGX is supported.
  --hypervisor     - print hypervisor vendor if detected.
  --quiet          - disable warnings
  --outfile=<file> - redirect all output to this file, instead of stdout
  --verbose, -v    - be extra verbose (more keys increase verbosiness level)
  --version        - print library version

Query switches (generate 1 line of output per switch; in order of appearance):
  --cpuid, --architecture, --feature-level, --purpose, --vendorstr, 
  --vendorid, --brandstr, --family, --model, --stepping, --extfamily, 
  --extmodel, --implementer, --variant, --part-num, --revision, --cores, 
  --logical, --total-cpus, --affi-mask, --l1d-cache, --l1i-cache, --cache, 
  --l2-cache, --l3-cache, --l4-cache, --l1d-assoc, --l1i-assoc, --l2-assoc, 
  --l3-assoc, --l4-assoc, --l1d-cacheline, --l1i-cacheline, --l2-cacheline, 
  --l3-cacheline, --l4-cacheline, --l1d-instances, --l1i-instances, 
  --l2-instances, --l3-instances, --l4-instances, --codename, --flags, 
  --clock, --clock-os, --clock-rdtsc, --clock-ic, --rdmsr, --rdmsr-raw, 
  --sse-size

If `-' is used for <file>, then stdin/stdout will be used instead of files.
When no options are present, the program behaves as if it was invoked with
  cpuid_tool "--save=raw.txt --outfile=report.txt --report --verbose"
```

## Contributing

Refer to the [dedicated page](CONTRIBUTING.md).

## Users

So far, I'm aware of the following projects which utilize libcpuid (listed alphabetically):

* CPU-X (https://github.com/TheTumultuousUnicornOfDarkness/CPU-X)
* fre:ac (https://www.freac.org/)
* I-Nex (https://github.com/i-nex/I-Nex)
* Multiprecision Computing Toolbox for MATLAB (https://www.advanpix.com/)
* ucbench (http://anrieff.net/ucbench)

We'd love to hear from you if you are also using libcpuid and want your project listed above.
