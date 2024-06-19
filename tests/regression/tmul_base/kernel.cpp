#include <stdint.h>
#include "vx_intrinsics.h"
#include "vx_print.h"
#include "vx_spawn.h"
#include "common.h"

void kernel_body(int task_id, kernel_arg_t* __UNIFORM__ arg) {
	TYPE* A = reinterpret_cast<TYPE*>(arg->A_addr);
	TYPE* B = reinterpret_cast<TYPE*>(arg->B_addr); //TODO
	TYPE* C = reinterpret_cast<TYPE*>(arg->C_addr);
	TYPE* aux = reinterpret_cast<TYPE*>(arg->aux_addr);
	int resac;
	uint32_t size = arg->size;
	uint32_t j = 0, i = 0, k = 0;	

	if(vx_thread_id() < 0){
		vx_printf("[DEVIC]: wid=%d tid=%d tmask=%tid\n", vx_warp_id(),vx_thread_id(), vx_thread_mask());
	}
	
	for(i = 0; i < size; i++){
		for(j = 0; j < size; j++){
			resac = 0;
			for(k = 0; k < size; k+=4){
				resac += A[i*size + k+vx_thread_id()] * B[(k+vx_thread_id())*size + j];
			}
			aux[vx_thread_id()] = resac; 
			vx_fence();
			if(vx_thread_id() == 0){
				C[i*size + j] = aux[0] + aux[1] + aux[2] + aux[3];
			}
		}
	}
}

int main() {
	kernel_arg_t* arg = (kernel_arg_t*)KERNEL_ARG_DEV_MEM_ADDR;
	vx_spawn_tasks(arg->num_tasks, (vx_spawn_tasks_cb)kernel_body, arg);
	return 0;
}
