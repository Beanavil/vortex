#include <iostream>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <chrono>
#include <vortex.h>
#include "common.h"

#define FLOAT_ULP 6

#define RT_CHECK(_expr)                                         \
   do {                                                         \
     int _ret = _expr;                                          \
     if (0 == _ret)                                             \
       break;                                                   \
     printf("Error: '%s' returned %d!\n", #_expr, (int)_ret);   \
	 cleanup();			                                              \
     exit(-1);                                                  \
   } while (false)

///////////////////////////////////////////////////////////////////////////////

template <typename Type>
class Comparator {};

template <>
class Comparator<int> {
public:
  static const char* type_str() {
    return "integer";
  }
  static int generate() {
    return rand();
  }
  static bool compare(int a, int b, int index, int errors) {
    if (a != b) {
      if (errors < 100) {
        printf("*** error: [%d] expected=%d, actual=%d\n", index, a, b);
      }
      return false;
    }
    return true;
  }
};

template <>
class Comparator<float> {
public:
  static const char* type_str() {
    return "float";
  }
  static int generate() {
    return static_cast<float>(rand()) / RAND_MAX;
  }
  static bool compare(float a, float b, int index, int errors) {
    union fi_t { float f; int32_t i; };
    fi_t fa, fb;
    fa.f = a;
    fb.f = b;
    auto d = std::abs(fa.i - fb.i);
    if (d > FLOAT_ULP) {
      if (errors < 100) {
        printf("*** error: [%d] expected=%f, actual=%f\n", index, a, b);
      }
      return false;
    }
    return true;
  }
};

const char* kernel_file = "kernel.bin";
const char* kernel_file_comp = "kernel_comp.cpp";

uint32_t size = 2;

vx_device_h device = nullptr;
std::vector<uint8_t> staging_buf;
kernel_arg_t kernel_arg = {};

static void show_usage() {
   std::cout << "Vortex Test." << std::endl;
   std::cout << "Usage: [-k: kernel] [-n size] [-h: help]" << std::endl;
}

static void parse_args(int argc, char **argv) {
  int c;
  while ((c = getopt(argc, argv, "n:k:h?")) != -1) {
    switch (c) {
    case 'n':
      size = atoi(optarg);
      break;
    case 'k':
      kernel_file = optarg;
      break;
    case 'h':
    case '?': {
      show_usage();
      exit(0);
    } break;
    default:
      show_usage();
      exit(-1);
    }
  }
}

void cleanup() {
  if (device) {
    vx_mem_free(device, kernel_arg.A_addr);
    vx_mem_free(device, kernel_arg.B_addr);
    vx_mem_free(device, kernel_arg.C_addr);
    vx_dev_close(device);
  }
}

int main(int argc, char *argv[]) {
  // parse command arguments
  parse_args(argc, argv);
  std::srand(50);

  // open device connection
  std::cout << "open device connection" << std::endl;
  RT_CHECK(vx_dev_open(&device));

  uint32_t num_points = size * size;
  uint32_t buf_size = num_points * sizeof(TYPE);

  // Upload program
  std::cout << "upload program" << std::endl;
  RT_CHECK(vx_upload_kernel_file(device, kernel_file));

  // allocate device memory
  std::cout << "allocate device memory" << std::endl;
  RT_CHECK(vx_mem_alloc(device, buf_size, VX_MEM_TYPE_GLOBAL, &kernel_arg.A_addr));
  RT_CHECK(vx_mem_alloc(device, buf_size, VX_MEM_TYPE_GLOBAL, &kernel_arg.B_addr));
  RT_CHECK(vx_mem_alloc(device, buf_size, VX_MEM_TYPE_GLOBAL, &kernel_arg.C_addr));;

  kernel_arg.num_tasks = 4; 
  kernel_arg.size = size;

  //TODO erase
  std::cout << "dev_src0=0x" << std::hex << kernel_arg.A_addr << std::endl;
  std::cout << "dev_src1=0x" << std::hex << kernel_arg.B_addr << std::endl;
  std::cout << "dev_dst=0x" << std::hex << kernel_arg.C_addr << std::endl;

  // allocate staging buffer
  std::cout << "allocate staging buffer" << std::endl;
  uint32_t alloc_size = std::max<uint32_t>(buf_size, sizeof(kernel_arg_t));
  staging_buf.resize(alloc_size);

  // upload kernel argument
  std::cout << "upload kernel argument" << std::endl;
  memcpy(staging_buf.data(), &kernel_arg, sizeof(kernel_arg_t));
  RT_CHECK(vx_copy_to_dev(device, KERNEL_ARG_DEV_MEM_ADDR, staging_buf.data(), sizeof(kernel_arg_t)));

  // generate source data
  std::vector<TYPE> src_A(num_points);
  std::vector<TYPE> src_B(num_points);
  std::vector<TYPE> src_C(num_points);
  std::vector<TYPE> refs(num_points);

  // ---- Matrix order for 2x2 submatrix multiplications -----
  for (uint32_t i = 0; i < size/2; ++i) {    
      for (uint32_t k = 0; k< size/2; ++k)
        for (uint32_t j = 0; j < 4; j ++)  {
          src_A[i*size*2 + k*4 + j] = j;
          src_B[i*size*2 + k*4 + j] = j;
        }
  }
  //Print 
  // for (uint32_t i = 0; i < size/2; ++i) {
  //   for (uint32_t j = 0; j < size*2; ++j) {
  //     std::cout << src_B[i* size*2 + j] << " ";  
  //   }
  //   std::cout << std::endl; 
  // }
  
  // upload source buffer0
  {
    std::cout << "upload source buffer0" << std::endl;
    auto buf_ptr = (TYPE*)staging_buf.data();
    for (uint32_t i = 0; i < num_points; ++i) {
      buf_ptr[i] = src_A[i];
    }
    RT_CHECK(vx_copy_to_dev(device, kernel_arg.A_addr, staging_buf.data(), buf_size));
  }

  // upload source buffer1
  {
    std::cout << "upload source buffer1" << std::endl;
    auto buf_ptr = (TYPE*)staging_buf.data();
    for (uint32_t i = 0; i < num_points; ++i) {
      buf_ptr[i] = src_B[i];
    }
    RT_CHECK(vx_copy_to_dev(device, kernel_arg.B_addr, staging_buf.data(), buf_size));
  }
  
  // clear destination buffer
  std::cout << "clear destination buffer" << std::endl;
  memset(staging_buf.data(), 0, num_points * sizeof(TYPE));
  RT_CHECK(vx_copy_to_dev(device, kernel_arg.C_addr, staging_buf.data(), buf_size));


  // --- Kernel launch ----
  auto time_start = std::chrono::high_resolution_clock::now();

  // start device
  std::cout << "start device" << std::endl;
  RT_CHECK(vx_start(device));

  // wait for completion
  std::cout << "wait for completion" << std::endl;
  RT_CHECK(vx_ready_wait(device, VX_MAX_TIMEOUT));

  auto time_end = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
  printf("\nResults\n - Size matrix: %dx%d\n - Elapsed time: %lg ms\n\n", size, size, elapsed);


  RT_CHECK(vx_copy_from_dev(device, staging_buf.data(), kernel_arg.C_addr, buf_size));

  cleanup();
  return 0;
}
