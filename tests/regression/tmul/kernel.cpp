#include <stdint.h>
#include "vx_intrinsics.h"
#include "vx_print.h"
#include "vx_spawn.h"
#include "common.h"

void kernel_body(int task_id, kernel_arg_t* __UNIFORM__ arg) {
	TYPE* A = reinterpret_cast<TYPE*>(arg->A_addr);
	TYPE* B = reinterpret_cast<TYPE*>(arg->B_addr);
	TYPE* C = reinterpret_cast<TYPE*>(arg->C_addr);
	vx_mload(A, B);
	vx_mmul();
	vx_mstore(C);

	vx_printf("DEVICE: C=%p C[0]=%d C[1]=%d C[2]=%d C[3]=%d\n", C, C[0], C[1], C[2], C[3]);
}

int main() {
	kernel_arg_t* arg = (kernel_arg_t*)KERNEL_ARG_DEV_MEM_ADDR;
	vx_spawn_tasks(arg->num_tasks, (vx_spawn_tasks_cb)kernel_body, arg);
	return 0;
}
