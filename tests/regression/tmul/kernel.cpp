#include <stdint.h>
#include "vx_intrinsics.h"
#include "vx_print.h"
#include "vx_spawn.h"
#include "common.h"


void add(TYPE* A, TYPE* B){


}

void kernel_body(int task_id, kernel_arg_t* __UNIFORM__ arg) {
	TYPE* A = reinterpret_cast<TYPE*>(arg->A_addr);
	TYPE* B = reinterpret_cast<TYPE*>(arg->B_addr);
	TYPE* C = reinterpret_cast<TYPE*>(arg->C_addr);

	vx_printf("[DEVICE]: wid=%d tid=%d\n tmask=%tid", vx_warp_id(),vx_thread_id(), vx_thread_mask());

	vx_mload_A_2x2(A, 4);
	vx_mload_B_2x2(B, 4);
	vx_mmul_2x2();
	vx_mstore_2x2 (C, 4);
}

int main() {
	kernel_arg_t* arg = (kernel_arg_t*)KERNEL_ARG_DEV_MEM_ADDR;
	vx_spawn_tasks(arg->num_tasks, (vx_spawn_tasks_cb)kernel_body, arg);
	return 0;
}
