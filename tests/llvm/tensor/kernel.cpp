#include <stdint.h>
#include "vx_intrinsics.h"
#include "vx_print.h"
#include "vx_spawn.h"
#include "common.h"

void kernel_body(int task_id, kernel_arg_t* __UNIFORM__ arg) {
	TYPE* A = reinterpret_cast<TYPE*>(arg->A_addr);
	TYPE* B = reinterpret_cast<TYPE*>(arg->B_addr);
	TYPE* C = reinterpret_cast<TYPE*>(arg->C_addr);
	vx_mload_a_2m2n2k_u32(A, 0);
	vx_mload_b_2m2n2k_u32(B, 0);
	vx_mmul_2m2n2k_u32();
	vx_mstore_c_2m2n2k_u32(C, 0);
}

int main() {
	kernel_arg_t* arg = (kernel_arg_t*)KERNEL_ARG_DEV_MEM_ADDR;
	vx_spawn_tasks(arg->num_tasks, (vx_spawn_tasks_cb)kernel_body, arg);
	return 0;
}
