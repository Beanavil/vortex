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
	int resac, threadID = vx_thread_id();
	uint32_t size = arg->size;
	uint32_t j = 0, i = 0, k = 0;	
	uint32_t partial;

	if(threadID < 0){
		vx_printf("[DEVIC]: wid=%d tid=%d tmask=%tid\n", vx_warp_id(),threadID, vx_thread_mask());
	}

	uint32_t blocksize = 2;
	uint32_t innerRow = threadID%2;
	uint32_t innerColumn;
	if(innerRow == 0){
		innerColumn = threadID==2;
	}else{
		innerColumn = threadID==3;
	}
	// vx_printf("Th %d - innerColumn=%d, innerRow=%d\n", threadID, innerColumn, innerRow);
	
	for(i = 0; i < size; i+=blocksize){
		for(j = 0; j < size; j+=blocksize){
			partial = 0;
			for(k = 0; k < size; k+=blocksize){
				partial += A[((i + innerRow)*size) + k]*B[(k*size + j + innerColumn)] 
						+ A[((i + innerRow)*size) + k + 1]*B[((k+1)*size + j + innerColumn)];
			}
			C[(i+innerRow)*size + j + innerColumn] = partial; 

		}
	}
}

int main() {
	kernel_arg_t* arg = (kernel_arg_t*)KERNEL_ARG_DEV_MEM_ADDR;
	vx_spawn_tasks(arg->num_tasks, (vx_spawn_tasks_cb)kernel_body, arg);
	return 0;
}


// vx_printf("Th %d - A1[%d]\n", threadID, ((i + innerRow)*size) + k);
// vx_printf("Th %d - B1[%d]\n", threadID, (k*size + j + innerColumn));
// vx_printf("Th %d - A2[%d]\n", threadID, ((i + innerRow)*size) + k + 1);
// vx_printf("Th %d - B2[%d]\n", threadID, ((k+1)*size + j + innerColumn));
				
