# Webassembly Linux Interface (WALI)

<p align="center">
  <img src="assets/main-logo-small.png?raw=true" alt="WebAssembly Linux Interface">
</p>

<h3 align="center">Bringing All of Linux to WebAssembly!</h3>
<div align="center">[ <a href="https://wasm-thin-kernel-interfaces.github.io/WALI/">Website</a> | <a href="https://dl.acm.org/doi/10.1145/3689031.3717470">Publication</a> | <a href="https://wasm-thin-kernel-interfaces.github.io/WALI/specification/">WALI Specification</a> ]</div>

## Quick Start

```shell
sudo ./install-deps.sh
# Full setup: toolchain configs + runtime + compiler + libc. 
# Use the `-r` option for only runtime build.
./quick_setup.sh

# Running programs 
## Native Linux Host
./iwasm -v=0 ./examples/precompiled/lua/lua.wasm
## Non-Linux Host (Docker Image)
docker build -t wali -f runtime.Dockerfile .
docker run --rm -it -w /dir -v $(pwd):/dir wali ./examples/precompiled/lua/lua.wasm
```

For more granular control, see the [detailed setup guide](#detailed-setup-guide).

## "Hello World": Build and Run

```shell
cd examples
# This script sets up standard build flags for the compiler toolchain. 
# For WALI binaries without main/start functions, refer to `print_nostart.c` instead
./compile-wali-standalone.sh -o print.wasm print.c
# Run the binary (or `./print.wasm` if miscellaneous binary format is setup)
../iwasm print.wasm
```
You can find more sample programs in [examples/mini](examples/mini/).


## Detailed Setup Guide

First, setup toolchain configs: 
```shell
# See `toolchains/README.md` if creating custom toolchains
python3 toolchains/gen_toolchains.py
```

The toolchain consists of two major parts:
* ***I want to run WALI programs!***: [WALI Engine](#wali-engine)
* ***I want to compile/build WALI programs!***: [Compile Toolchain](#compiler-toolchain)


## WALI Engine

We include a baseline implementation of WALI in WAMR. 
See [examples/precompiled](examples/precompiled) for runnable WALI binaries.

### Native Linux Host
```shell
# Install dependencies (or equivalent packages without apt)
sudo ./install-deps.sh 
git submodule update --init wasm-micro-runtime
# Generates `iwasm` symlink in root directory
make iwasm
```

### [Recommended] Miscellaneous Binary Format
Execute  `.wasm` files can be executed like ELF files (e.g. `./bash.wasm --norc`)!
This is **necessary** to build some applications that execute intermediate binaries.
```shell
# Specify '-p' option to register with systemd-binfmt to survive system reboots.
sudo ./toolchains/binfmt/binfmt_register.sh -p
# Run like a native binary!
./examples/precompiled/lua/lua.wasm
```

More info on miscellaneous binfmts can be found [here](https://docs.kernel.org/admin-guide/binfmt-misc.html)


### Non-Linux Hosts (Docker Environment)

```shell
# Building the image
docker build -t wali -f runtime.Dockerfile .
# Running binaries with the image
docker run --rm -it -w /dir -v (pwd):/dir wali <prog.wasm> <args..>
```


## Compile Toolchain

```shell
make compiler SLIM=1  # SLIM only includes a minimal set of llvm bins.
# Build musl-libc (sysroot)
git submodule update --init wali-musl
make libc
```

#### [Optional] AoT Compiler
Generate faster ahead-of time (AoT) compiled executables. For the WAMR 
implementation, additional details can be found on the [WAMR compiler docs](https://github.com/SilverLineFramework/wasm-micro-runtime/tree/wali/wamr-compiler):
```shell
# Build and use wamrc
make wamrc
wamrc --enable-multi-thread -o lua.aot ./examples/precompiled/lua/lua.wasm
# Run like a normal wasm file
./iwasm lua.aot
```

## Test Suite

To build and run the unit test suite:
```shell
cd tests
# Ensure runtime, libc, and compiler toolchains were built prior to this
make -j && python3 run_tests.py
```

## Additional Resources
* [Compiler ports](compiler_ports/README.md) of WALI for other languages.
* [Zenodo](https://zenodo.org/records/14829424) Ubuntu 22.04 VM artifact from publication.

## Notes
* We currently support only x86\_64, aarch64, and riscv64. More ISAs will be added to this list as necessary.
