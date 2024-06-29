// Copyright © 2019-2023
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Runtime
#include "vx_axi.h"

// Vortex
#include <VX_config.h>
#include <VX_types.h>
#include <vortex.h>
#include <vortex_afu.h>

// STL
#include <cstdlib>
#include <memory>

#include "baremetal.h"

///////////////////////////////////////////////////////////////////////////////

// clang-format off
#define MMIO_CTL_ADDR       0x00
#define MMIO_DEV_ADDR       0x10
#define MMIO_ISA_ADDR       0x1C
#define MMIO_DCR_ADDR       0x28
#define MMIO_SCP_ADDR       0x34
#define MMIO_MEM_ADDR       0x40
#define MMIO_BASE_ADDR      0xA0000000

#define CTL_AP_START        (1<<0)
#define CTL_AP_DONE         (1<<1)
#define CTL_AP_IDLE         (1<<2)
#define CTL_AP_READY        (1<<3)
#define CTL_AP_RESET        (1<<4)
#define CTL_AP_RESTART      (1<<7)

#ifndef MEM_TRANSF_WIDTH
#define MEM_TRANSF_WIDTH    (32 / 8)
#endif

#define RAM_PAGE_SIZE        4096
#define DEFAULT_DEVICE_INDEX 0
#define DEFAULT_XCLBIN_PATH  "vortex_afu.xclbin"
#define KERNEL_NAME          "vortex_afu"
// clang-format on

#ifndef NDEBUG
#define DBGPRINT(format, ...)                                                  \
  do {                                                                         \
    printf("[VXDRV] " format "", ##__VA_ARGS__);                               \
  } while (0)
#else
#define DBGPRINT(format, ...) ((void)0)
#endif

#define CHECK_HANDLE(handle, _expr, _cleanup)                                  \
  auto handle = _expr;                                                         \
  if (handle == nullptr) {                                                     \
    printf("[VXDRV] Error: '%s' returned NULL!\n", #_expr);                    \
    _cleanup                                                                   \
  }

#define CHECK_ERR(_expr, _cleanup)                                             \
  do {                                                                         \
    auto err = _expr;                                                          \
    if (err == 0)                                                              \
      break;                                                                   \
    printf("[VXDRV] Error: '%s' returned %d!\n", #_expr, (int)err);            \
    _cleanup                                                                   \
  } while (false)

using namespace vortex;

///////////////////////////////////////////////////////////////////////////////

class vx_device {
public:
  vx_device(uint64_t base_addr) : vx_axi(base_addr) {}

  ~vx_device() {}

  // 32-bit register operations
  int read_register32(uint64_t addr, uint64_t &value) {
    CHECK_ERR(vx_axi.read(addr, (uint32_t &)value), { return -1; });
    DBGPRINT("*** read_register32: addr=0x%x, value=0x%x\n", addr, value);
    return 0;
  }

  int write_register32(uint64_t addr, uint64_t value) {
    CHECK_ERR(vx_axi.write(addr, (uint32_t)value), { return -1; });
    DBGPRINT("*** write_register32: addr=0x%x, value=0x%x\n", addr, value);
    return 0;
  }

  // 64-bit register operations
  int read_register64(uint64_t addr, uint64_t *value) {
    CHECK_ERR(vx_axi.read64(addr, value), { return -1; });
    DBGPRINT("*** read_register: addr=0x%x, value=0x%lx\n", addr, *value);
    return 0;
  }

  int write_register64(uint64_t addr, uint64_t value) {
    CHECK_ERR(vx_axi.write64(addr, value), { return -1; });
    DBGPRINT("*** write_register: addr=0x%x, value=0x%lx\n", addr, value);
    return 0;
  }

  /// Initializes a device.
  /// Returns 0 if sucess, and -1 otherwise.
  int init() {
    CHECK_ERR(this->write_register32(MMIO_CTL_ADDR, CTL_AP_RESET),
              { return -1; });

    CHECK_ERR(this->read_register32(MMIO_DEV_ADDR, (uint32_t *)&this->dev_caps),
              { return -1; });

    CHECK_ERR(this->read_register32(MMIO_DEV_ADDR + 4,
                                    (uint32_t *)&this->dev_caps + 1),
              { return -1; });

    CHECK_ERR(this->read_register32(MMIO_ISA_ADDR, (uint32_t *)&this->isa_caps),
              { return -1; });

    CHECK_ERR(this->read_register32(MMIO_ISA_ADDR + 4,
                                    (uint32_t *)&this->isa_caps + 1),
              { return -1; });

    // Assume 8GB as default
    this->global_mem_size = GLOBAL_MEM_SIZE;

    this->global_mem = std::make_shared<vortex::MemoryAllocator>(
        ALLOC_BASE_ADDR, ALLOC_MAX_ADDR, RAM_PAGE_SIZE, CACHE_BLOCK_SIZE);

    uint64_t local_mem_size = 0;
    vx_dev_caps(this, VX_CAPS_LOCAL_MEM_SIZE, &local_mem_size);
    if (local_mem_size <= 1) {
      this->local_mem = std::make_shared<vortex::MemoryAllocator>(
          SMEM_BASE_ADDR, local_mem_size, RAM_PAGE_SIZE, 1);
    }

    return 0;
  }

  /// Allocates local/global memory on device.
  /// Returns 0 if success, and -1 otherwise.
  int mem_alloc(uint64_t size, int type, uint64_t *dev_addr) {
    uint64_t asize = aligned_size(size, CACHE_BLOCK_SIZE);

    uint64_t addr;

    if (type == VX_MEM_TYPE_GLOBAL) {
      CHECK_ERR(global_mem_->allocate(asize, &addr), { return -1; });
    } else if (type == VX_MEM_TYPE_LOCAL) {
      CHECK_ERR(local_mem_->allocate(asize, &addr), { return -1; });
    } else {
      return -1;
    }
    *dev_addr = addr;
    return 0;
  }

  /// Writes memory from host to device.
  /// Returns 0 if successful, -1 otherwise.
  int upload(uint64_t dev_addr, uint32_t *host_ptr, uint64_t asize) {
    // Wait until device is ready
    if (vx_ready_wait((vx_device_h)this, VX_MAX_TIMEOUT) != 0) {
      return -1;
    }

    for (uint64_t i = 0; i < asize / MEM_TRANSF_WIDTH; ++i) {
      // Set up transaction data
      uint64_t value = host_ptr[i];
      CHECK_ERR(write_register(MMIO_CMD_DATA, (uint32_t)value), { return -1; });
      CHECK_ERR(write_register(MMIO_CMD_ADDR,
                               (uint32_t)dev_addr + i * MEM_TRANSF_WIDTH),
                { return -1; });
      CHECK_ERR(write_register(MMIO_CMD_SIZE, MEM_TRANSF_WIDTH),
                { return -1; });
      CHECK_ERR(write_register(MMIO_CMD_TYPE, CMD_MEM_WRITE), { return -1; });

      // Wait until device finishes
      if (vx_ready_wait((vx_device_h)this, VX_MAX_TIMEOUT) != 0) {
        return -1;
      }
    }
    return 0;
  }

  /// Reads device memory from host.
  /// Returns 0 if successful, -1 otherwise.
  int download(uint32_t *host_ptr, uint64_t dev_addr, uint64_t asize) {
    // Wait until device is ready
    if (vx_ready_wait((vx_device_h)this, VX_MAX_TIMEOUT) != 0) {
      return -1;
    }

    for (uint64_t i = 0; i < asize / MEM_TRANSF_WIDTH; ++i) {
      // Set up transaction data
      uint64_t value;
      CHECK_ERR(write_register(MMIO_CMD_ADDR,
                               (uint32_t)dev_addr + i * MEM_TRANSF_WIDTH),
                { return -1; });
      CHECK_ERR(write_register(MMIO_CMD_SIZE, MEM_TRANSF_WIDTH),
                { return -1; });
      CHECK_ERR(write_register(MMIO_CMD_TYPE, CMD_MEM_READ), { return -1; });

      // Wait until device finishes
      if (vx_ready_wait((vx_device_h)this, VX_MAX_TIMEOUT) != 0) {
        return -1;
      }

      // Read data from device
      CHECK_ERR(read_register32(MMIO_DATA_READ, value), { return -1; });
      host_ptr[i] = value;
    }
    return 0;
  }

  vx_axi vx_axi;
  uint64_t global_mem_size;
  std::shared_ptr<vortex::MemoryAllocator> global_mem;
  std::shared_ptr<vortex::MemoryAllocator> local_mem;
  DeviceConfig dcrs;
  uint64_t dev_caps;
  uint64_t isa_caps;
};

///////////////////////////////////////////////////////////////////////////////

extern int vx_dev_caps(vx_device_h device_h, uint32_t caps_id,
                       uint64_t *value) {
  if (device_h == nullptr) {
    return -1;
  }

  vx_device *device = ((vx_device *)device_h);

  switch (caps_id) {
  case VX_CAPS_VERSION:
    *value = (device->dev_caps >> 0) & 0xff;
    break;
  case VX_CAPS_NUM_THREADS:
    *value = (device->dev_caps >> 8) & 0xff;
    break;
  case VX_CAPS_NUM_WARPS:
    *value = (device->dev_caps >> 16) & 0xff;
    break;
  case VX_CAPS_NUM_CORES:
    *value = (device->dev_caps >> 24) & 0xffff;
    break;
  case VX_CAPS_CACHE_LINE_SIZE:
    *value = CACHE_BLOCK_SIZE;
    break;
  case VX_CAPS_GLOBAL_MEM_SIZE:
    *value = device->global_mem_size;
    break;
  case VX_CAPS_LOCAL_MEM_SIZE:
    *value = 1ull << ((device->dev_caps >> 40) & 0xff);
    break;
  case VX_CAPS_KERNEL_BASE_ADDR:
    *value = (uint64_t(device->dcrs.read32(VX_DCR_BASE_STARTUP_ADDR1)) << 32) |
             device->dcrs.read32(VX_DCR_BASE_STARTUP_ADDR0);
    break;
  case VX_CAPS_ISA_FLAGS:
    *value = device->isa_caps;
    break;
  default:
    fprintf(stderr, "[VXDRV] Error: invalid caps id: %d\n", caps_id);
    std::abort();
    return -1;
  }

  return 0;
}

extern int vx_dev_open(vx_device_h *device_h) {
  if (device_h == nullptr) {
    return -1;
  }

  vx_device *device;

  device = new vx_device(MMIO_BASE_ADDR);
  if (device == nullptr) {
    return -1;
  }

  // Initialize device
  CHECK_ERR(device->init(), { return -1; });

  CHECK_ERR(dcr_initialize(device), {
    delete device;
    return -1;
  });

#ifdef DUMP_PERF_STATS
  perf_add_device(device);
#endif

  *device_h = device;

  DBGPRINT("device creation complete!\n");

  return 0;
}

extern int vx_dev_close(vx_device_h device_h) {
  if (device_h == nullptr) {
    return -1;
  }

  auto device = ((vx_device *)device_h);

#ifdef DUMP_PERF_STATS
  perf_remove_device(device_h);
#endif

  delete device;

  DBGPRINT("device destroyed!\n", NULL);

  return 0;
}

extern int vx_mem_alloc(vx_device_h device_h, uint64_t size, int type,
                        uint64_t *dev_addr) {
  if (device_h == nullptr || dev_addr == nullptr || 0 == size) {
    return -1;
  }

  auto device = ((vx_device *)device_h);
  return device->mem_alloc(size, type, dev_addr);
}

extern int vx_mem_free(vx_device_h device_h, uint64_t dev_addr) {
  if (device_h == nullptr) {
    return -1;
  }

  if (0 == dev_addr) {
    return 0;
  }

  auto device = ((vx_device *)device_h);
  if (dev_addr >= SMEM_BASE_ADDR) {
    return device->local_mem->release(dev_addr);
  } else {
    return device->global_mem->release(dev_addr);
  }
}

extern int vx_mem_info(vx_device_h device_h, int type, uint64_t *mem_free,
                       uint64_t *mem_used) {
  if (device_h == nullptr)
    return -1;

  auto device = ((vx_device *)device_h);
  if (type == VX_MEM_TYPE_GLOBAL) {
    if (mem_free)
      *mem_free = device->global_mem->free();
    if (mem_used)
      *mem_used = device->global_mem->allocated();
  } else if (type == VX_MEM_TYPE_LOCAL) {
    if (mem_free)
      *mem_free = device->local_mem->free();
    if (mem_used)
      *mem_free = device->local_mem->allocated();
  } else {
    return -1;
  }
  return 0;
}

extern int vx_copy_to_dev(vx_device_h device_h, uint64_t dev_addr,
                          const void *host_ptr, uint64_t size) {
  if (device_h == nullptr) {
    return -1;
  }

  auto device = (vx_device *)device_h;

  // check alignment
  if (!is_aligned(dev_addr, CACHE_BLOCK_SIZE)) {
    return -1;
  }

  auto asize = aligned_size(size, CACHE_BLOCK_SIZE);

  // bound checking
  if (dev_addr + asize > device->global_mem_size) {
    return -1;
  }

  CHECK_ERR(device->upload(dev_addr, (uint32_t *)host_ptr, asize),
            { return -1; });

  DBGPRINT("COPY_TO_DEV: dev_addr=0x%lx, host_addr=0x%lx, size=%ld bytes\n",
           dev_addr, (uintptr_t)host_ptr, size);

  return 0;
}

extern int vx_copy_from_dev(vx_device_h device_h, void *host_ptr,
                            uint64_t dev_addr, uint64_t size) {
  if (device_h == nullptr) {
    return -1;
  }

  auto device = (vx_device *)device_h;

  // check alignment
  if (!is_aligned(dev_addr, CACHE_BLOCK_SIZE)) {
    return -1;
  }

  auto asize = aligned_size(size, CACHE_BLOCK_SIZE);

  // bound checking
  if (dev_addr + asize > device->global_mem_size) {
    return -1;
  }

  CHECK_ERR(device->download((uint32_t *)host_ptr, dev_addr, asize),
            { return -1; });

  DBGPRINT("COPY_FROM_DEV: dev_addr=0x%lx, host_addr=0x%lx, size=%ld bytes\n",
           dev_addr, (uintptr_t)host_ptr, asize);

  return 0;
}

extern int vx_start(vx_device_h device_h) {
  if (device_h == nullptr) {
    return -1;
  }

  auto device = (vx_device *)device_h;

  CHECK_ERR(device->write_register32(MMIO_CTL_ADDR, CTL_AP_START),
            { return -1; });

  DBGPRINT("START\n", NULL);

  return 0;
}

extern int vx_ready_wait(vx_device_h device_h, uint64_t timeout) {
  if (device_h == nullptr) {
    return -1;
  }

  auto device = (vx_device *)device_h;

  struct timespec sleep_time;

#ifndef NDEBUG
  sleep_time.tv_sec = 1;
  sleep_time.tv_nsec = 0;
#else
  sleep_time.tv_sec = 0;
  sleep_time.tv_nsec = 1000000;
#endif

  // to milliseconds
  uint64_t sleep_time_ms =
      (sleep_time.tv_sec * 1000) + (sleep_time.tv_nsec / 1000000);

  for (;;) {
    uint32_t status = 0;
    CHECK_ERR(device->read_register32(MMIO_CTL_ADDR, &status), { return -1; });
    bool is_done = (status & CTL_AP_DONE) == CTL_AP_DONE;
    if (is_done || 0 == timeout) {
      break;
    }
    nanosleep(&sleep_time, nullptr);
    timeout -= sleep_time_ms;
  };

  return 0;
}

extern int vx_dcr_write(vx_device_h device_h, uint32_t addr, uint64_t value) {
  if (device_h == nullptr) {
    return -1;
  }

  auto device = (vx_device *)device_h;

  CHECK_ERR(device->write_register32(MMIO_DCR_ADDR, addr), { return -1; });
  CHECK_ERR(device->write_register32(MMIO_DCR_ADDR + 4, value), { return -1; });

  // save the value
  DBGPRINT("DCR_WRITE: addr=0x%x, value=0x%lx\n", addr, value);
  device->dcrs.write(addr, value);

  return 0;
}
