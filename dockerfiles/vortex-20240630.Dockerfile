FROM ubuntu:20.04

ARG VORTEX_GITHUB=https://github.com/Beanavil/vortex
ARG VORTEX_DIR=vortex

ARG LLVM_GITHUB=https://github.com/Beanavil/vortex-llvm
ARG LLVM_DIR=llvm-vortex

# Install dependencies
RUN export DEBIAN_FRONTEND=noninteractive \
    &&  apt-get update -qq \
    &&  apt-get install -y tzdata \
    &&  ln -fs /usr/share/zoneinfo/Europe/Madrid /etc/localtime \
    &&  dpkg-reconfigure --frontend noninteractive tzdata
RUN apt-get install -y git make gcc sudo iproute2 gawk net-tools \
    libssl-dev gnupg wget gcc-multilib lsb-release locales \
    build-essential zlib1g-dev libtinfo-dev libncurses5 uuid-dev \
    libboost-serialization-dev libpng-dev libhwloc-dev cmake ninja-build

RUN locale-gen en_US.UTF-8 && update-locale

# Make a root-capable user
RUN adduser --disabled-password --gecos '' admin \
    && usermod -aG sudo admin \
    && echo "admin ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# Replace the bash
RUN sudo rm /bin/sh \
    && sudo ln -s /bin/bash /bin/sh

# Change to added user
USER admin
ENV HOME /home/admin
ENV LANG en_US.UTF-8
WORKDIR /home/admin

# 1. Get Vortex codebase
RUN git clone --recursive ${VORTEX_GITHUB} ${VORTEX_DIR}

# 2. Install dependencies
RUN sudo ~/${VORTEX_DIR}/ci/toolchain_install.sh --all

# 3. Set up prebuilt toolchain
ARG TOOLDIR=/opt
ENV TOOLDIR=${TOOLDIR}

ENV VERILATOR_ROOT $TOOLDIR/verilator
ENV PATH $VERILATOR_ROOT/bin:$PATH
ENV SV2V_PATH $TOOLDIR/sv2v
ENV PATH $SV2V_PATH/bin:$PATH
ENV YOSYS_PATH $TOOLDIR/yosys
ENV PATH $YOSYS_PATH/bin:$PATH

# 4. Set up custom LLVM
RUN cd ~ && git clone ${LLVM_GITHUB} ${LLVM_DIR}
RUN cd ~/${LLVM_DIR} \
    && cmake -G Ninja -S llvm -B build -DLLVM_INSTALL_UTILS=ON -DCMAKE_INSTALL_PREFIX=/opt/${LLVM_DIR} -DCMAKE_BUILD_TYPE=Release -DLLVM_DEFAULT_TARGET_TRIPLE="riscv32-unknown-elf" -DLLVM_TARGETS_TO_BUILD="RISCV" -DLLVM_ENABLE_PROJECTS="clang" \
    && ninja -C build install

# 5. Build Vortex's sources
RUN cd ~/${VORTEX_DIR} && make -j $(nproc)