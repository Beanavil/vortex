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

#ifndef __VX_INTRINSICS_H__
#define __VX_INTRINSICS_H__

#include <VX_config.h>
#include <VX_types.h>

#if defined(__clang__)
#define __UNIFORM__   __attribute__((annotate("vortex.uniform")))
#else
#define __UNIFORM__
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __ASSEMBLY__
#define __ASM_STR(x)	x
#else
#define __ASM_STR(x)	#x
#endif

#define RISCV_CUSTOM0   0x0B
#define RISCV_CUSTOM1   0x2B
#define RISCV_CUSTOM2   0x5B
#define RISCV_CUSTOM3   0x7B

#define csr_read(csr) ({                        \
	unsigned __r;	               		        \
	__asm__ __volatile__ ("csrr %0, %1" : "=r" (__r) : "i" (csr)); \
	__r;							            \
})

#define csr_write(csr, val)	({                  \
	unsigned __v = (unsigned)(val);             \
	if (__builtin_constant_p(val) && __v < 32)  \
        __asm__ __volatile__ ("csrw %0, %1"	:: "i" (csr), "i" (__v));  \
    else                                        \
        __asm__ __volatile__ ("csrw %0, %1"	:: "i" (csr), "r" (__v));  \
})

#define csr_swap(csr, val) ({                   \
    unsigned __r;                               \
	unsigned __v = (unsigned)(val);	            \
	if (__builtin_constant_p(val) && __v < 32)  \
        __asm__ __volatile__ ("csrrw %0, %1, %2" : "=r" (__r) : "i" (csr), "i" (__v)); \
    else                                        \
        __asm__ __volatile__ ("csrrw %0, %1, %2" : "=r" (__r) : "i" (csr), "r" (__v)); \
	__r;						                \
})

#define csr_read_set(csr, val) ({               \
	unsigned __r;                               \
	unsigned __v = (unsigned)(val);	            \
    if (__builtin_constant_p(val) && __v < 32)  \
	    __asm__ __volatile__ ("csrrs %0, %1, %2" : "=r" (__r) : "i" (csr), "i" (__v)); \
    else                                        \
        __asm__ __volatile__ ("csrrs %0, %1, %2" : "=r" (__r) : "i" (csr), "r" (__v)); \
	__r;							            \
})

#define csr_set(csr, val) ({                    \
	unsigned __v = (unsigned)(val);	            \
    if (__builtin_constant_p(val) && __v < 32)  \
	    __asm__ __volatile__ ("csrs %0, %1"	:: "i" (csr), "i" (__v));  \
    else                                        \
        __asm__ __volatile__ ("csrs %0, %1"	:: "i" (csr), "r" (__v));  \
})

#define csr_read_clear(csr, val) ({             \
	unsigned __r;                               \
	unsigned __v = (unsigned)(val);	            \
    if (__builtin_constant_p(val) && __v < 32)  \
	    __asm__ __volatile__ ("csrrc %0, %1, %2" : "=r" (__r) : "i" (csr), "i" (__v)); \
    else                                        \
        __asm__ __volatile__ ("csrrc %0, %1, %2" : "=r" (__r) : "i" (csr), "r" (__v)); \
	__r;							            \
})

#define csr_clear(csr, val)	({                  \
	unsigned __v = (unsigned)(val);             \
	if (__builtin_constant_p(val) && __v < 32)  \
        __asm__ __volatile__ ("csrc %0, %1"	:: "i" (csr), "i" (__v)); \
    else                                        \
        __asm__ __volatile__ ("csrc %0, %1"	:: "i" (csr), "r" (__v)); \
})

// Conditional move
inline unsigned vx_cmov(unsigned c, unsigned t, unsigned f) {
    unsigned ret;
    asm volatile (".insn r4 %1, 1, 0, %0, %2, %3, %4" : "=r"(ret) : "i"(RISCV_CUSTOM1), "r"(c), "r"(t), "r"(f));
    return ret;
}

// Set thread mask
inline void vx_tmc(unsigned thread_mask) {
    asm volatile (".insn r %0, 0, 0, x0, %1, x0" :: "i"(RISCV_CUSTOM0), "r"(thread_mask));
}

// disable all threads in the current warp
inline void vx_tmc_zero() {
    asm volatile (".insn r %0, 0, 0, x0, x0, x0" :: "i"(RISCV_CUSTOM0));
}

// switch execution to single thread zero
inline void vx_tmc_one() {
    asm volatile (
        "li a0, 1\n\t"  // Load immediate value 1 into a0 (x10) register
        ".insn r %0, 0, 0, x0, a0, x0" :: "i"(RISCV_CUSTOM0) 
        : "a0"          // Indicate that a0 (x10) is clobbered
    );
}

// Set thread predicate
inline void vx_pred(unsigned condition, unsigned thread_mask) {
    asm volatile (".insn r %0, 5, 0, x0, %1, %2" :: "i"(RISCV_CUSTOM0), "r"(condition), "r"(thread_mask));
}

typedef void (*vx_wspawn_pfn)();

// Spawn warps
inline void vx_wspawn(unsigned num_warps, vx_wspawn_pfn func_ptr) {
    asm volatile (".insn r %0, 1, 0, x0, %1, %2" :: "i"(RISCV_CUSTOM0), "r"(num_warps), "r"(func_ptr));
}

// Split on a predicate
inline unsigned vx_split(unsigned predicate) {
    unsigned ret;
    asm volatile (".insn r %1, 2, 0, %0, %2, x0" : "=r"(ret) : "i"(RISCV_CUSTOM0), "r"(predicate));
    return ret;
}

// Join
inline void vx_join(unsigned stack_ptr) {
    asm volatile (".insn r %0, 3, 0, x0, %1, x0" :: "i"(RISCV_CUSTOM0), "r"(stack_ptr));
}

// Warp Barrier
inline void vx_barrier(unsigned barried_id, unsigned num_warps) {
    asm volatile (".insn r %0, 4, 0, x0, %1, %2" :: "i"(RISCV_CUSTOM0), "r"(barried_id), "r"(num_warps));
}

// NOTE: check `info -f riscv64-gnu-toolchain/share/info/as.info | less`
// search for `RISC-V Instruction Formats`

// TODO erase 
// inline void vx_mload_a_2x2_x24(int* addr, unsigned int stride) {

//     //  +--------------+-----+-------+----+---------+
//     //  | simm12[11:0] | rs1 | func3 | rd | opcode6 |
//     //  +--------------+-----+-------+----+---------+
//     //  31             20    15      12   7         0

//     // 'I type: .insn i opcode6, func3, rd, simm12(rs1)'
//     asm volatile (".insn i %0, 0, x24, %2(%1)" :: "i"(RISCV_CUSTOM2), "r"(addr), "i"(stride));
// }
// inline void vx_mload_b_2x2_x24(int* addr, int stride) {

//     //  +--------------+-----+-------+----+---------+
//     //  | simm12[11:0] | rs1 | func3 | rd | opcode6 |
//     //  +--------------+-----+-------+----+---------+
//     //  31             20    15      12   7         0

//     // 'I type: .insn i opcode6, func3, rd, simm12(rs1)'
//     asm volatile (".insn i %0, 1, x26, %2(%1)" :: "i"(RISCV_CUSTOM2), "r"(addr), "i"(stride));
// }
// inline void vx_mmul_2x2_x24() {

//     //    +-------+-----+-----+-------+----+---------+
//     //    | func7 | rs2 | rs1 | func3 | rd | opcode6 |
//     //    +-------+-----+-----+-------+----+---------+
//     //    31      25    20    15      12   7        0

//     //'R type: .insn r opcode6, func3, func7, rd, rs1, rs2'
//     asm volatile (".insn r %0, 4, 0, x24, x24, zero" :: "i"(RISCV_CUSTOM2));
// }
// inline void vx_mstore_2x2_x24(int* output, unsigned int stride) {

//     //  +--------------+-----+-----+-------+-------------+---------+
//     //  | simm12[11:5] | rs2 | rs1 | func3 | simm12[4:0] | opcode6 |
//     //  +--------------+-----+-----+-------+-------------+---------+
//     //  31             25    20    15      12            7         0

//     //  S type: .insn s opcode6, func3, rs2, simm12(rs1)
//     asm volatile (".insn s %1, 6, x24, %2(%0)" :: "r"(output), "i"(RISCV_CUSTOM2),"i"(stride));
// }


// Special matrix operations
// Matrices load
inline int vx_mload_a_2m2n2k(int* addr, unsigned int stride) {
    int val;
    //  +--------------+-----+-------+----+---------+
    //  | simm12[11:0] | rs1 | func3 | rd | opcode6 |
    //  +--------------+-----+-------+----+---------+
    //  31             20    15      12   7         0

    // 'I type: .insn i opcode6, func3, rd, simm12(rs1)'
    asm volatile (".insn i %1, 0, %0, %3(%2)" : "=tr"(val) : "i"(RISCV_CUSTOM2), "r"(addr), "i"(stride));
    return val;
}

inline int vx_mload_b_2m2n2k(int* addr, int stride) {
    int val;
    //  +--------------+-----+-------+----+---------+
    //  | simm12[11:0] | rs1 | func3 | rd | opcode6 |
    //  +--------------+-----+-------+----+---------+
    //  31             20    15      12   7         0

    // 'I type: .insn i opcode6, func3, rd, simm12(rs1)'
    asm volatile (".insn i %1, 1, %0, %3(%2)" : "=tr"(val) : "i"(RISCV_CUSTOM2), "r"(addr), "i"(stride));
    return val;
}

inline int vx_mload_c_2m2n2k(int* addr, int stride) {
    int val;
    //  +--------------+-----+-------+----+---------+
    //  | simm12[11:0] | rs1 | func3 | rd | opcode6 |
    //  +--------------+-----+-------+----+---------+
    //  31             20    15      12   7         0

    // 'I type: .insn i opcode6, func3, rd, simm12(rs1)'
    asm volatile (".insn i %1, 2, %0, %3(%2)" : "=r"(val) : "i"(RISCV_CUSTOM2), "r"(addr), "i"(stride));
    return val;
}

inline int vx_mmul_2m2n2k(int A, int B) {
    int res;
    //    +-------+-----+-----+-------+----+---------+
    //    | func7 | rs2 | rs1 | func3 | rd | opcode6 |
    //    +-------+-----+-----+-------+----+---------+
    //    31      25    20    15      12   7        0

    //'R type: .insn r opcode6, func3, func7, rd, rs1, rs2'
    asm volatile (".insn r %3, 3, 0, %0, %1, %2" : "=r"(res): "tr"(A), "tr"(B), "i"(RISCV_CUSTOM2));
    return res;
}

inline int vx_madd_2m2n2k(int a, int b) {
    //      +-------+-----+-----+-------+----+---------+
    //      | func7 | rs2 | rs1 | func3 | rd | opcode6 |
    //      +-------+-----+-----+-------+----+---------+
    //      31      25    20    15      12   7        0
    //
    //    R type: .insn r opcode6, func3, func7, rd, rs1, rs2
    int res;

    asm volatile (".insn r %3, 4, 0, %0, %1, %2" : "=r"(res) : "r"(a), "r"(b), "i"(RISCV_CUSTOM2));

    return res;
}

inline void vx_mstore_d_2m2n2k(int res, int* output, unsigned int stride) {
    //  +--------------+-----+-----+-------+-------------+---------+
    //  | simm12[11:5] | rs2 | rs1 | func3 | simm12[4:0] | opcode6 |
    //  +--------------+-----+-----+-------+-------------+---------+
    //  31             25    20    15      12            7         0

    //  S type: .insn s opcode6, func3, rs2, simm12(rs1)
    asm volatile (".insn s %1, 5, %0, %3(%2)" :: "r"(res), "i"(RISCV_CUSTOM2), "r"(output),"i"(stride));
}

// Return current thread identifier
inline int vx_thread_id() {
    int ret;
    asm volatile ("csrr %0, %1" : "=r"(ret) : "i"(VX_CSR_THREAD_ID));
    return ret;
}

// Return current warp identifier
inline int vx_warp_id() {
    int ret;
    asm volatile ("csrr %0, %1" : "=r"(ret) : "i"(VX_CSR_WARP_ID));
    return ret;
}

// Return current core identifier
inline int vx_core_id() {
    int ret;
    asm volatile ("csrr %0, %1" : "=r"(ret) : "i"(VX_CSR_CORE_ID));
    return ret;
}

// Return current thread mask
inline int vx_thread_mask() {
    int ret;
    asm volatile ("csrr %0, %1" : "=r"(ret) : "i"(VX_CSR_THREAD_MASK));
    return ret;
}

// Return number of active warps
inline int vx_active_warps() {
    int ret;
    asm volatile ("csrr %0, %1" : "=r"(ret) : "i"(VX_CSR_WARP_MASK));
    return ret;
}

// Return the number of threads per warp
inline int vx_num_threads() {
    int ret;
    asm volatile ("csrr %0, %1" : "=r"(ret) : "i"(VX_CSR_NUM_THREADS));
    return ret;
}

// Return the number of warps per core
inline int vx_num_warps() {
    int ret;
    asm volatile ("csrr %0, %1" : "=r"(ret) : "i"(VX_CSR_NUM_WARPS));
    return ret;   
}

// Return the number of cores per cluster
inline int vx_num_cores() {
    int ret;
    asm volatile ("csrr %0, %1" : "=r"(ret) : "i"(VX_CSR_NUM_CORES));
    return ret;
}

// Return the hart identifier (thread id accross the processor)
inline int vx_hart_id() {
    int ret;
    asm volatile ("csrr %0, %1" : "=r"(ret) : "i"(VX_CSR_MHARTID));
    return ret;
}

inline void vx_fence() {
    asm volatile ("fence iorw, iorw");
}

#ifdef __cplusplus
}
#endif

#endif // __VX_INTRINSICS_H__
