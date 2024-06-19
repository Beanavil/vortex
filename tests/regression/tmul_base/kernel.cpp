#include <stdint.h>
#include "vx_intrinsics.h"
#include "vx_print.h"
#include "vx_spawn.h"
#include "common.h"


void add(TYPE* A, TYPE* B){


}

void kernel_body(int task_id, kernel_arg_t* __UNIFORM__ arg) {
	TYPE* A = reinterpret_cast<TYPE*>(arg->A_addr);
	TYPE* B = reinterpret_cast<TYPE*>(arg->B_addr); //TODO
	TYPE* C = reinterpret_cast<TYPE*>(arg->C_addr);
	TYPE* aux = reinterpret_cast<TYPE*>(arg->aux_addr);
	int resac;
	uint32_t size = arg->size;
	uint32_t j = 0, i = 0, k = 0;	

	
	vx_printf("[DEVIC]: wid=%d tid=%d tmask=%tid\n", vx_warp_id(),vx_thread_id(), vx_thread_mask());

	// for ( i = 0; i < size; ++i) {
    // 	for ( j = 0; j < size; ++j) {
	// 		vx_printf("%d ", B[i* size + j]);
    // 	}
	// 	vx_printf("\n");
  	// }	return;
	
	for(i = 0; i < size; i++){
		for(j = 0; j < size; j++){
			resac = 0;
			for(k = 0; k < size; k+=4){
				resac += A[i*size + k+vx_thread_id()] * B[(k+vx_thread_id())*size + j];
				// vx_printf("thid %d - Rest: %d - A[%d]:%d , B[%d]%d \n", 
				// vx_thread_id(), resac, i*size + k+vx_thread_id(),  A[i*size + k+vx_thread_id()], 
				// (k+vx_thread_id())*size + j, B[(k+vx_thread_id())*size + j]);
			}
			
			aux[vx_thread_id()] = resac; 
			vx_fence();
			if(vx_thread_id() == 0){
				C[i*size + j] = aux[0] + aux[1] + aux[2] + aux[3];
			}
		}
	}

	
	
	// for (i = 0; i < size/2; i++){
	// 	for (k = 0; k < size/2; k++){
	// 		j = 0;
	// 		rsA = vx_mload_a_2m2n2k(&A[i*size*2 + j*size], 4);
	// 		rsB = vx_mload_b_2m2n2k(&B[j*size*2 + k*4], 4);
	// 		resac = vx_mmul_2m2n2k(rsA, rsB);

	// 		for (j = 1; j < size/2; j+=1){ 

	// 			rsA = vx_mload_a_2m2n2k(&A[i*size*2 + j*size], 4);
	// 			rsB = vx_mload_b_2m2n2k(&B[j*size*2 + k*4], 4);
	// 			// Mult matrices and acumulate
	// 			resmul = vx_mmul_2m2n2k(rsA, rsB);
	// 			resac = vx_madd_2m2n2k(resac, resmul);
	// 		}
	// 		vx_mstore_d_2m2n2k(resac, &C[i*size*2 + k*4], 4);
	// 	}
	// }	
}

int main() {
	kernel_arg_t* arg = (kernel_arg_t*)KERNEL_ARG_DEV_MEM_ADDR;
	vx_spawn_tasks(arg->num_tasks, (vx_spawn_tasks_cb)kernel_body, arg);
	return 0;
}
