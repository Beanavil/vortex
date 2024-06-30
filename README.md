# AOHW 2024

[![Build Status](https://travis-ci.com/vortexgpgpu/vortex.svg?branch=master)](https://travis-ci.com/vortexgpgpu/vortex)
[![codecov](https://codecov.io/gh/vortexgpgpu/vortex/branch/master/graph/badge.svg)](https://codecov.io/gh/vortexgpgpu/vortex)

## Team information

- Team number: `AOHW-200`
- Project name: RISC-V based GPU
- Link to YouTube Video(s):
- University name: Universitat Politècnica de Catalunya (UPC)
- Participant(s):
  - Javier Beiro Piñón
  - Beatriz Navidad Vilches
  - Nicolás Zhilie Zhao
- Supervisor: Dr. Leonidas Kosmidis

## Vortex GPGPU

Vortex is a full-stack open-source RISC-V GPGPU. For our submission to the AOHW2024, we have added support for tensor instructions for loading, storing and performing multiplication and addition of matrices. We have also added warp-level intrinsics that make use of these instructions.

### Specifications

- Supports RISC-V RV32IMAF and RV64IMAFD
- Microarchitecture:
  - configurable number of cores, warps, and threads.
  - configurable number of ALU, FPU, LSU, and SFU units per core.
  - configurable pipeline issue width.
  - optional shared memory, L1, L2, and L3 caches.
  - default configuration:
    - 1 core, 4 warps and 4 threads/warp
    - 4 ALU, 4 FPU, 4 LSU and 4 SFU units per core
    - issue width of 4
    - L1 enabled, L2 and L3 disabled
- Software:
  - OpenCL 1.2 Support.
- Supported FPGAs:
  - Altera Arria 10
  - Altera Stratix 10
  - Xilinx Alveo U50, U250, U280
  - Xilinx Versal VCK5000

### Directory structure

- `ci`: Continuous integration scripts.
- `docs`: [Documentation](docs/index.md).
- `hw`: Hardware sources.
- `kernel`: RISC-V device runtime.
- `miscs`: Miscellaneous resources.
- `runtime`: Host drivers implementations.
- `sim`: Simulators repository.
- `tests`: Tests repository.

### Build Requirements

#### Supported OS Platforms

- Ubuntu 18.04, 20.04
- CentOS 7

#### Toolchain Dependencies

- [POCL](http://portablecl.org/)
  [LLVM](https://llvm.org/)
- [Our Customized LLVM](https://github.com/Beanavil/vortex-llvm)
- [RISCV-GNU-TOOLCHAIN](https://github.com/riscv-collab/riscv-gnu-toolchain)
- [Verilator](https://www.veripool.org/verilator)
- [FpNew](https://github.com/pulp-platform/fpnew.git)
- [SoftFloat](https://github.com/ucb-bar/berkeley-softfloat-3.git)
- [Ramulator](https://github.com/CMU-SAFARI/ramulator.git)
- [Yosys](https://github.com/YosysHQ/yosys)
- [Sv2v](https://github.com/zachjs/sv2v)


### Build Instructions

We already ship a docker image based on Ubuntu 20.04 that has the Vortex repository cloned and all the dependencies installed. See []().

The build process is described below.

#### Within docker container
It's only necessary to build Vortex's sources:

```bash
cd vortex
make -s -j $(nproc)
```

#### From scratch

1. Get Vortex codebase

    ```bash
    git clone --recursive https://github.com/Beanavil/vortex.git vortex
    cd vortex
    ```

2. Install dependencies

    ```bash
    sudo apt-get install build-essential zlib1g-dev libtinfo-dev libncurses5 uuid-dev libboost-serialization-dev libpng-dev libhwloc-dev ninja-build cmake
    ```

    and upgrade gcc to 11:

    ```bash
    sudo apt-get install gcc-11 g++-11
    ```

      Multiple gcc versions on Ubuntu can be managed with update-alternatives, e.g.:

    ```bash
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 9
    sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 9
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 11
    sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 11
    ```

3. Set up prebuilt toolchain

    By default, the toolchain will be installed  to the `/opt` folder, which
    requires sudo access. You can install the toolchain to a different
    location of your choice by setting `TOOLDIR` (e.g. `export TOOLDIR=$HOME/tools`).

    ```bash
    export TOOLDIR=/opt
    ```
    ```bash
    ./ci/toolchain_install.sh --all
    ```
    ```bash
    source ./ci/toolchain_env.sh
    ```

4. Set up custom LLVM

    ```bash
    git clone https://github.com/Beanavil/vortex-llvm llvm-vortex && cd llvm-vortex
    ```
    ```bash
    cmake -G Ninja -S llvm -B build -DLLVM_INSTALL_UTILS=ON -DCMAKE_INSTALL_PREFIX=$TOOLDIR/llvm-vortex -DCMAKE_BUILD_TYPE=Release -DLLVM_DEFAULT_TARGET_TRIPLE="riscv32-unknown-elf" -DLLVM_TARGETS_TO_BUILD="RISCV" -DLLVM_ENABLE_PROJECTS="clang"
    ```
    ```bash
    ninja -C build install
    ```

5. Build Vortex's sources

    ```bash
    make -s -j $(nproc)
    ```

### Execute the tensor core test

For executing the tensor core test for $2\times 2$ matrices with hardware simulation:

```bash
./ci/blackbox.sh --driver=rtlsim --app=tmul --args="-n2"
```
