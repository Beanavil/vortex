#include <stdint.h>
#include "vx_intrinsics.h"
#include "vx_print.h"
#include "vx_spawn.h"
#include "common.h"

void kernel_body(int task_id, kernel_arg_t* __UNIFORM__ arg) {
	TYPE* A = reinterpret_cast<TYPE*>(arg->A_addr);
	TYPE* B = reinterpret_cast<TYPE*>(arg->B_addr);
	TYPE* C = reinterpret_cast<TYPE*>(arg->C_addr);
	/*TYPE* D = reinterpret_cast<TYPE*>(arg->D_addr);*/

	/*vx_mload_A_2x2(A, 4);*/
	/*vx_mload_B_2x2(B, 4);*/
	/*vx_mmul_2x2();*/
	/**/
	/*//  x24 <- vx_mload_A_2x2_x24(D, 4);*/
	/*asm volatile (".insn i %0, 0, x24, %2(%1)" :: "i"(0x5b), "r"(D), "i"(4));*/
	/*vx_fence();*/
	/*vx_madd_2x2();*/
	/**/
	/*// vx_mstore_2x2(C, 4);  */
	/*// C <- x24[0..threads-1];*/
	/*asm volatile (".insn s %1, 6, x24, %2(%0)" :: "r"(C), "i"(0x5b),"i"(4));*/
	vx_mload_a_2m2n2k_u32(A, 4);
	vx_mload_b_2m2n2k_u32(B, 4);
	vx_mmul_2m2n2k_u32();
	vx_mstore_c_2m2n2k_u32 (C, 4);
}

int main() {
	kernel_arg_t* arg = (kernel_arg_t*)KERNEL_ARG_DEV_MEM_ADDR;
	vx_spawn_tasks(arg->num_tasks, (vx_spawn_tasks_cb)kernel_body, arg);
	return 0;
}
