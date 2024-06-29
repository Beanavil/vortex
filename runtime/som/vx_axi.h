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

struct vx_axi {
  vx_axi(uint64_t base_addr);

  int read32(uint64_t addr, uint32_t &value) const;
  int read64(uint64_t addr, uint64_t &value) const;

  int write32(uint64_t addr, uint32_t value);
  int write64(uint64_t addr, uint64_t value);

private:
  uint64_t m_base_addr;
};
