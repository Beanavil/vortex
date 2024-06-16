#include <stdint.h>
#include "vx_intrinsics.h"
#include "vx_print.h"
#include "vx_spawn.h"
#include "common.h"

void kernel_body(int task_id, kernel_arg_t* __UNIFORM__ arg) {
	TYPE* A = reinterpret_cast<TYPE*>(arg->A_addr);
	TYPE* B = reinterpret_cast<TYPE*>(arg->B_addr);
	TYPE* C = reinterpret_cast<TYPE*>(arg->C_addr);
	TYPE* D = reinterpret_cast<TYPE*>(arg->D_addr);

	int rsA = vx_mload_a_2m2n2k(A, 4);
	int rsB = vx_mload_b_2m2n2k(B, 4);
	int rsC = vx_mload_c_2m2n2k(C, 4);
	vx_fence();
	int resMul = vx_mmul_2m2n2k(rsA, rsB);
	int result = vx_madd_2m2n2k(resMul, rsC);
	vx_mstore_d_2m2n2k(result, D, 4);
}

int main() {
	kernel_arg_t* arg = (kernel_arg_t*)KERNEL_ARG_DEV_MEM_ADDR;
	vx_spawn_tasks(arg->num_tasks, (vx_spawn_tasks_cb)kernel_body, arg);
	return 0;
}
