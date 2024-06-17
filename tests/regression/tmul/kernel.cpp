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

	// vx_printf("-----Valor %d\n", A[2]);

	vx_printf("[DEVIC]: wid=%d tid=%d tmask=%tid\n", vx_warp_id(),vx_thread_id(), vx_thread_mask());

	// vx_mload_A_2x2(A, 4);
	// vx_mload_B_2x2(B, 4);
	// vx_mmul_2x2();
	// vx_mstore_2x2 (C, 4);

	// vx_mload_A_2x2_x24(A, 4);
	// vx_mload_B_2x2_x24(B, 4);
	// vx_mmul_2x2_x24();
	// vx_mstore_2x2_x24 (C, 4);

	// TODO send the size 
	uint32_t size = 8;
	vx_printf("Size %d\n", size);
	for (uint32_t i = 0; i < size/2; i++){
		for (uint32_t k = 0; k < size/2; k++){
			for (uint32_t j = 0; j < size/2; j+=2){
				vx_printf("Valor i=%d; valor j=%d; valor k=%d\n", i, j, k);
				vx_printf("Matrix A -> posición: %d Valor: %d\n",
						i*size*2 + j*4, A[i*size*2 + j*size]);
				vx_printf("Matrix B -> posición: %d Valor: %d\n",
						j*size*2 + k*4, A[j*size*2 + k*4]); //TODO 
				vx_printf("Matrix A' -> posición: %d Valor: %d\n",
						i*size*2 + (j*4 + 4), A[i*size*2 + (j*4 + 4)]);
				vx_printf("Matrix B' -> posición: %d Valor: %d\n",
						((j+1)*size)*2 + k*4 , A[((j+1)*size)*2 + k*4]);

				// vx_mload_A_2x2(A[i*size*2 + j*size], 4);
				// vx_mload_B_2x2(B[j*size*2 + k*4 + i*size], 4);
				vx_mload_A_2x2(&A[i*size*2 + j*4], 4);
				vx_mload_B_2x2_x24(&A[j*size*2 + k*4], 4);
				
				vx_mload_A_2x2(&A[i*size*2 + j*4], 4);
				vx_mload_B_2x2_x24(&A[((j+1)*size)*2 + k*4], 4);

				vx_mmul_2x2();
				vx_mmul_2x2_x24();

				//ADD x24 y x28 -> x24

				//ADD x24 y x20 -> //TODO el contenido de x20 tiene que estar a 0 (la primera iteración no acumula solo aplasta)
				
			}
			//Store x20 en las posición que digan A y B 
			vx_printf("Fuera bucle 1\n");
		}
		vx_printf("Fuera bucle 2\n");
	}	
}

int main() {
	kernel_arg_t* arg = (kernel_arg_t*)KERNEL_ARG_DEV_MEM_ADDR;
	vx_spawn_tasks(arg->num_tasks, (vx_spawn_tasks_cb)kernel_body, arg);
	return 0;
}
