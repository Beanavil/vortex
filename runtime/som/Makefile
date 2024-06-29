CXX = aarch64-none-elf-g++
AR = aarch64-none-elf-ar
XLEN ?= 32
HW_PATH = $(VORTEX_HOME)/hw

CXXFLAGS += -std=c++11 -Wall -Wextra -pedantic -Wfatal-errors
CXXFLAGS += -I../include -I../common -I$(HW_PATH) -I.
CXXFLAGS += -DSTARTUP_ADDR=0xA0000000
#CXXFLAGS += -DSTACK_BASE_ADDR=$(STACK_BASE_ADDR)
CXXFLAGS += -DXLEN=$(XLEN)
CXXFLAGS += -O2 -DNDEBUG
#CXXFLAGS += -fPIC
#LDFLAGS += -shared -ldl

PROJECT = libvortex.a

all: $(PROJECT)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(PROJECT): vortex.o vx_axi.o  ../common/utils.o 
	$(AR) rcs $(PROJECT) $^


