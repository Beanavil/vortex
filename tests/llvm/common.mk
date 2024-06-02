XLEN ?= 32

TOOLDIR ?= /opt

TARGET ?= opaesim

XRT_SYN_DIR ?= ../../../hw/syn/xilinx/xrt
XRT_DEVICE_INDEX ?= 0

ifeq ($(XLEN),64)
RISCV_TOOLCHAIN_PATH ?= $(TOOLDIR)/riscv64-gnu-toolchain
VX_CFLAGS += -march=rv64imafd -mabi=lp64d
STARTUP_ADDR ?= 0x180000000
else
RISCV_TOOLCHAIN_PATH ?= $(TOOLDIR)/riscv-gnu-toolchain
VX_CFLAGS += -march=rv32imaf -mabi=ilp32f
STARTUP_ADDR ?= 0x80000000
endif

RISCV_PREFIX ?= riscv$(XLEN)-unknown-elf
RISCV_SYSROOT ?= $(RISCV_TOOLCHAIN_PATH)/$(RISCV_PREFIX)

VORTEX_RT_PATH ?= $(realpath ../../../runtime)
VORTEX_KN_PATH ?= $(realpath ../../../kernel)

FPGA_BIN_DIR ?= $(VORTEX_RT_PATH)/opae

# LLVM
LLVM_VORTEX ?= $(TOOLDIR)/llvm-vortex

LLVM_CFLAGS += --sysroot=$(RISCV_SYSROOT)
LLVM_CFLAGS += --gcc-toolchain=$(RISCV_TOOLCHAIN_PATH)
LLVM_CFLAGS += -Xclang -target-feature -Xclang +vortex

# Vortex-LLVM
VX_CC  = $(LLVM_VORTEX)/bin/clang $(LLVM_CFLAGS)
VX_CXX = $(LLVM_VORTEX)/bin/clang++ $(LLVM_CFLAGS)
VX_DP  = $(TOOLDIR)/vortex-llvm/bin/llvm-objdump
VX_CP  = $(TOOLDIR)/vortex-llvm/bin/llvm-objcopy
VX_OPT = $(TOOLDIR)/vortex-llvm/bin/opt
VX_LLC = $(TOOLDIR)/vortex-llvm/bin/llc

VX_CFLAGS += -v -O3 -std=c++17
VX_CFLAGS += -mcmodel=medany -fno-rtti -fno-exceptions -nostartfiles -fdata-sections -ffunction-sections
VX_CFLAGS += -I$(VORTEX_KN_PATH)/include -I$(VORTEX_KN_PATH)/../hw
VX_CFLAGS += -DNDEBUG -DLLVM_VORTEX

VX_LDFLAGS += -Wl,-Bstatic,--gc-sections,-T,$(VORTEX_KN_PATH)/linker/vx_link$(XLEN).ld,--defsym=STARTUP_ADDR=$(STARTUP_ADDR) $(VORTEX_KN_PATH)/libvortexrt.a

# Clang
CXXFLAGS += -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors
CXXFLAGS += -I$(VORTEX_RT_PATH)/include -I$(VORTEX_KN_PATH)/../hw

LDFLAGS += -L$(VORTEX_RT_PATH)/stub -lvortex

# Debugigng
ifdef DEBUG
	CXXFLAGS += -g -O0
else
	CXXFLAGS += -O2 -DNDEBUG
endif

ifeq ($(TARGET), fpga)
	OPAE_DRV_PATHS ?= libopae-c.so
else
ifeq ($(TARGET), asesim)
	OPAE_DRV_PATHS ?= libopae-c-ase.so
else
ifeq ($(TARGET), opaesim)
	OPAE_DRV_PATHS ?= libopae-c-sim.so
endif
endif
endif

# Generating output files
all: $(PROJECT) kernel.bin kernel.dump kernel.ll kernel.o #kernel.ll.opt

# LLVM object file dump from binary, should be the same as kernel.o
kernel.dump: kernel.elf
	$(VX_DP) -D kernel.elf > kernel.dump

# LLMV copy of clang's binary
kernel.bin: kernel.elf
	$(VX_CP) -O binary kernel.elf kernel.bin

# Clang output binary
kernel.elf: $(VX_SRCS)
	$(VX_CXX) $(VX_CFLAGS) $(VX_SRCS) $(VX_LDFLAGS) -o kernel.elf

## C++ ##
# LLVM IR
kernel.ll:
	$(VX_CXX) -S -emit-llvm $(VX_CFLAGS) $(VX_SRCS) $(VX_LDFLAGS)

# LLVM optimized IR
# kernel.ll.opt:
# 	$(VX_OPT) --passes=dot-cfg -o kernel.ll

# LLVM object file for RISC-V 32 backend
kernel.o: kernel.ll
	$(VX_LLC) -march=riscv32 kernel.ll -o kernel.o

$(PROJECT): $(SRCS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

run-simx: $(PROJECT) kernel.bin
	LD_LIBRARY_PATH=$(VORTEX_RT_PATH)/simx:$(LD_LIBRARY_PATH) ./$(PROJECT) $(OPTS)

run-opae: $(PROJECT) kernel.bin
	SCOPE_JSON_PATH=$(FPGA_BIN_DIR)/scope.json OPAE_DRV_PATHS=$(OPAE_DRV_PATHS) LD_LIBRARY_PATH=$(VORTEX_RT_PATH)/opae:$(LD_LIBRARY_PATH) ./$(PROJECT) $(OPTS)

run-rtlsim: $(PROJECT) kernel.bin
	LD_LIBRARY_PATH=$(VORTEX_RT_PATH)/rtlsim:$(LD_LIBRARY_PATH) ./$(PROJECT) $(OPTS)

run-xrt: $(PROJECT) kernel.bin
ifeq ($(TARGET), hw)
	SCOPE_JSON_PATH=$(FPGA_BIN_DIR)/scope.json XRT_INI_PATH=$(XRT_SYN_DIR)/xrt.ini EMCONFIG_PATH=$(FPGA_BIN_DIR) XRT_DEVICE_INDEX=$(XRT_DEVICE_INDEX) XRT_XCLBIN_PATH=$(FPGA_BIN_DIR)/vortex_afu.xclbin LD_LIBRARY_PATH=$(XILINX_XRT)/lib:$(VORTEX_RT_PATH)/xrt:$(LD_LIBRARY_PATH) ./$(PROJECT) $(OPTS)
else
	XCL_EMULATION_MODE=$(TARGET) XRT_INI_PATH=$(XRT_SYN_DIR)/xrt.ini EMCONFIG_PATH=$(FPGA_BIN_DIR) XRT_DEVICE_INDEX=$(XRT_DEVICE_INDEX) XRT_XCLBIN_PATH=$(FPGA_BIN_DIR)/vortex_afu.xclbin LD_LIBRARY_PATH=$(XILINX_XRT)/lib:$(VORTEX_RT_PATH)/xrt:$(LD_LIBRARY_PATH) ./$(PROJECT) $(OPTS)
endif

.depend: $(SRCS)
	$(CXX) $(CXXFLAGS) -MM $^ > .depend;

clean:
	rm -rf $(PROJECT) *.o .depend

clean-all: clean
	rm -rf *.elf *.bin *.dump *.ll *.ll.opt *.o

ifneq ($(MAKECMDGOALS),clean)
    -include .depend
endif
