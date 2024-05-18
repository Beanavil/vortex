// Copyright © 2019-2023
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

`include "VX_define.vh"

module VX_ibuffer import VX_gpu_pkg::*; #(
    parameter CORE_ID = 0
) (
    input wire          clk,
    input wire          reset,

    // inputs
    VX_decode_if.slave  decode_if,

    // outputs
    VX_ibuffer_if.master ibuffer_if [`ISSUE_WIDTH]
);
    `UNUSED_PARAM (CORE_ID)
    localparam ISW_WIDTH  = `LOG2UP(`ISSUE_WIDTH);
    localparam DATAW = `UUID_WIDTH + ISSUE_WIS_W + `NUM_THREADS + `XLEN + 1 + `EX_BITS + `INST_OP_BITS + `INST_MOD_BITS + 1 + 1 + `XLEN + (`NR_BITS * 4) + 1;
    wire [`ISSUE_WIDTH-1:0] ibuf_ready_in;
    wire [`ISSUE_WIDTH-1:0] readies;

    wire [ISW_WIDTH-1:0] decode_isw = wid_to_isw(decode_if.data.wid);
    wire [ISSUE_WIS_W-1:0] decode_wis = wid_to_wis(decode_if.data.wid);
    assign decode_if.ready = ibuf_ready_in[decode_isw] && (&readies);


    VX_decode_if decode_micro_if[`ISSUE_WIDTH]();

    reg [`ISSUE_WIDTH-1:0] state_n, state_q;
    reg [`ISSUE_WIDTH-1:0][1:0] mmul_count_q, mmul_count_n;
    localparam ISSUE_NORMAL = 1'b0;
    localparam ISSUE_MMUL = 1'b1;

    wire is_mmul;
    assign is_mmul = decode_if.data.op_type == `INST_ALU_MMUL;

    for (genvar i = 0; i < `ISSUE_WIDTH; ++i) begin
        assign readies[i] = !((state_q[i] == ISSUE_NORMAL && is_mmul) || (state_q[i] == ISSUE_MMUL && mmul_count_q[i] == 2'h2));
        assign decode_micro_if[i].valid = decode_if.valid;
        assign decode_micro_if[i].data.op_type = `INST_ALU_ADD;
        assign decode_micro_if[i].data.ex_type = `EX_ALU;
        assign decode_micro_if[i].data.uuid = decode_if.data.uuid;
        assign decode_micro_if[i].data.wid = decode_if.data.wid;
        assign decode_micro_if[i].data.tmask = decode_if.data.tmask;
        assign decode_micro_if[i].data.op_mod = mmul_count_q[i] < 2'h2 ? 3'b010 : 3'b000;
        // assign decode_micro_if[i].data.wb = mmul_count_q[i] >= 2'h2;
        assign decode_micro_if[i].data.wb = decode_if.data.wb;
        assign decode_micro_if[i].data.use_PC = decode_if.data.use_PC;
        assign decode_micro_if[i].data.use_imm = decode_if.data.use_imm;
        assign decode_micro_if[i].data.PC = decode_if.data.PC;
        assign decode_micro_if[i].data.imm = decode_if.data.imm;
        assign decode_micro_if[i].data.rd =  mmul_count_q[i] < 2'h2 ? decode_if.data.rd + `NR_BITS'(mmul_count_q[i]): decode_if.data.rd;
        assign decode_micro_if[i].data.rs1 = mmul_count_q[i] < 2'h2 ? decode_if.data.rs1 + `NR_BITS'(mmul_count_q[i]) : decode_if.data.rd;
        assign decode_micro_if[i].data.rs2 = mmul_count_q[i] < 2'h2 ? decode_if.data.rs1 + `NR_BITS'(mmul_count_q[i]) + 'h2 : decode_if.data.rd + 1'b1;
        assign decode_micro_if[i].data.rs3 = decode_if.data.rs3;
        assign decode_micro_if[i].data.is_mstore = decode_if.data.is_mstore;

        VX_elastic_buffer #(
            .DATAW   (DATAW),
            .SIZE    (`IBUF_SIZE),
            .OUT_REG (1)
        ) instr_buf (
            .clk      (clk),
            .reset    (reset),
            .valid_in (decode_if.valid && decode_isw == i),
            .ready_in (ibuf_ready_in[i] ) ,
            .data_in  (
            (is_mmul || state_q[i] == ISSUE_MMUL) ?
                {
                decode_micro_if[i].data.uuid,
                decode_wis,
                decode_micro_if[i].data.tmask,
                decode_micro_if[i].data.ex_type,
                decode_micro_if[i].data.op_type,
                decode_micro_if[i].data.op_mod,
                decode_micro_if[i].data.wb,
                decode_micro_if[i].data.use_PC,
                decode_micro_if[i].data.use_imm,
                decode_micro_if[i].data.PC,
                decode_micro_if[i].data.imm,
                decode_micro_if[i].data.rd, 
                decode_micro_if[i].data.rs1, 
                decode_micro_if[i].data.rs2, 
                decode_micro_if[i].data.rs3,
                decode_micro_if[i].data.is_mstore
                }:
                {
                decode_if.data.uuid,
                decode_wis,
                decode_if.data.tmask,
                decode_if.data.ex_type,
                decode_if.data.op_type,
                decode_if.data.op_mod,
                decode_if.data.wb,
                decode_if.data.use_PC,
                decode_if.data.use_imm,
                decode_if.data.PC,
                decode_if.data.imm,
                decode_if.data.rd, 
                decode_if.data.rs1, 
                decode_if.data.rs2, 
                decode_if.data.rs3,
                decode_if.data.is_mstore
                }

                ),
            .data_out(ibuffer_if[i].data),
            .valid_out (ibuffer_if[i].valid),
            .ready_out(ibuffer_if[i].ready)
        );        
    `ifndef L1_ENABLE
        assign decode_if.ibuf_pop[i] = ibuffer_if[i].valid && ibuffer_if[i].ready;
    `endif

        always @(*) begin
                assign state_n[i] = state_q[i];
                assign mmul_count_n[i] = mmul_count_q[i];
                if ((state_q[i] == ISSUE_NORMAL && is_mmul) || (state_q[i] == ISSUE_MMUL)) begin
                    assign mmul_count_n[i] = mmul_count_q[i] + 1'b1;

                case (state_q[i])
                    ISSUE_NORMAL: begin
                        mmul_count_n[i] = '0;
                        if (is_mmul) begin
                            if (ibuf_ready_in[i]) begin
                                state_n[i] = ISSUE_MMUL;
                                mmul_count_n[i] = mmul_count_q[i] + 1'b1;
                            end
                        end
                    end
                    ISSUE_MMUL: begin
                         if (ibuf_ready_in[i]) begin
                            if (mmul_count_q[i] == 'h2) begin
                                state_n[i] = ISSUE_NORMAL;
                            end else begin
                                mmul_count_n[i] = mmul_count_q[i] + 1'b1;
                            end
                        end
                    end
                endcase
            end
        end

        always @(posedge clk) begin
                if (reset) begin
                    mmul_count_q[i] <= '0;
                    state_q[i] <= ISSUE_NORMAL;
                end else begin
                    mmul_count_q[i] <= mmul_count_n[i];
                    state_q[i] <= state_n[i];
                end
        end
    end
endmodule
