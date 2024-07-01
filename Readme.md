# libcpuid

libcpuid provides CPU identification. Supported CPU architectures are:
- x86:
  - 32-bit CPUs (IA-32, also known as i386, i486, i586 and i686)
  - 64-bit CPUs (x86_64, also known as x64, AMD64, and Intel 64)
- ARM (since v0.7.0):
  - 64-bit CPUs (ARM64, also known as AArch64)

For details about the programming API, you might want to
take a look at the project's website on sourceforge
(https://libcpuid.sourceforge.net). There you'd find a short
[tutorial](https://libcpuid.sourceforge.net/documentation.html), as well
as the full [API reference](https://anrieff.github.io/libcpuid).

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

Basic example to build and install libcpuid by using CMake:
```shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
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
