`ifndef VX_GPU_DCACHE_REQ
`define VX_GPU_DCACHE_REQ

`include "../generic_cache/VX_cache_config.vh"

interface VX_gpu_dcache_req_if #(
	parameter NUM_REQUESTS = 32
) ();

	// Core request
	wire [NUM_REQUESTS-1:0]         core_req_valid;
	wire [NUM_REQUESTS-1:0][2:0]    core_req_read;
	wire [NUM_REQUESTS-1:0][2:0]    core_req_write;
	wire [NUM_REQUESTS-1:0][31:0]   core_req_addr;
	wire [NUM_REQUESTS-1:0][31:0]   core_req_data;	
	wire                            core_req_ready;

	// Core request Meta data
    wire [4:0]                      core_req_rd;
    wire [NUM_REQUESTS-1:0][1:0]    core_req_wb;
    wire [`NW_BITS-1:0]             core_req_warp_num;
    wire [31:0]                     core_req_pc;	

endinterface

`endif