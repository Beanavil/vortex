#include <stdint.h>
#include "vx_intrinsics.h"
#include "vx_print.h"
#include "vx_spawn.h"
#include "common.h"

void kernel_body(int task_id, kernel_arg_t* __UNIFORM__ arg) {
	TYPE* A = reinterpret_cast<TYPE*>(arg->A_addr);
	TYPE* B = reinterpret_cast<TYPE*>(arg->B_addr); //TODO
	TYPE* C = reinterpret_cast<TYPE*>(arg->C_addr);
	int rsA, rsB;
	int resmul, resac;

	if(vx_thread_id() < 0){
		vx_printf("[DEVIC]: wid=%d tid=%d tmask=%tid\n", vx_warp_id(),vx_thread_id(), vx_thread_mask());
	}
	
	uint32_t size = arg->size;
	uint32_t j = 0, i = 0, k = 0;
	
	for (i = 0; i < size/2; i++){
		for (k = 0; k < size/2; k++){
			j = 0;
			rsA = vx_mload_a_2m2n2k(&A[i*size*2 + j*size], 4);
			rsB = vx_mload_b_2m2n2k(&B[j*size*2 + k*4], 4);
			resac = vx_mmul_2m2n2k(rsA, rsB);

			for (j = 1; j < size/2; j+=1){ 

				rsA = vx_mload_a_2m2n2k(&A[i*size*2 + j*size], 4);
				rsB = vx_mload_b_2m2n2k(&B[j*size*2 + k*4], 4);
				// Mult matrices and acumulate
				resmul = vx_mmul_2m2n2k(rsA, rsB);
				resac = vx_madd_2m2n2k(resac, resmul);
			}
			vx_mstore_d_2m2n2k(resac, &C[i*size*2 + k*4], 4);
		}
	}	
}

int main() {
	kernel_arg_t* arg = (kernel_arg_t*)KERNEL_ARG_DEV_MEM_ADDR;
	vx_spawn_tasks(arg->num_tasks, (vx_spawn_tasks_cb)kernel_body, arg);
	return 0;
}
