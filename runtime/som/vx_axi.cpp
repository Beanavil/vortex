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


// STL
#include <cstdint>
#include "vx_axi.h"

vx_axi::vx_axi(uint64_t base_addr) : m_base_addr(base_addr) {}

/// Reads a value from an address \p addr mapped to <tt>m_base_addr + addr</tt>.
/// Returns 0 if success, and -1 otherwise.
int vx_axi::read32(uint64_t addr, uint32_t* value) const {
  if (value == nullptr) {
    return -1;
  }
  const uint32_t *mapped_addr =
      reinterpret_cast<const uint32_t *>(m_base_addr + addr);
  *value = *mapped_addr;
  return 0;
}

int vx_axi::read64(uint64_t addr, uint64_t* value) const {
  if (value == nullptr) {
    return -1;
  }
  const uint32_t *low = reinterpret_cast<const uint32_t *>(m_base_addr + addr);
  const uint32_t *high =
      reinterpret_cast<const uint32_t *>(m_base_addr + addr + 4);
  *value = ((uint64_t)*high << 32) | ((uint64_t)*low & 0x0ffffffff);
  return 0;
}

/// Writes a value to an address \p addr mapped to <tt>m_base_addr + addr</tt>.
/// Returns 0 if success, and -1 otherwise.
int vx_axi::write32(uint64_t addr, uint32_t value) {
  uint32_t *mapped_addr = reinterpret_cast<uint32_t *>(m_base_addr + addr);
  *mapped_addr = value;
  return 0;
}

int vx_axi::write64(uint64_t addr, uint64_t value) {
  uint32_t low = value & 0x0ffffffff;
  uint32_t high = value >> 32;
  uint32_t *addr_low = reinterpret_cast<uint32_t *>(m_base_addr + addr);
  uint32_t *addr_high = reinterpret_cast<uint32_t *>(m_base_addr + addr + 4);
  *addr_low = low;
  *addr_high = high;
  return 0;
}
