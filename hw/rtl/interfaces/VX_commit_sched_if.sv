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

interface VX_commit_sched_if ();

    wire [`ISSUE_WIDTH-1:0] committed;
    wire [`ISSUE_WIDTH-1:0][`NW_WIDTH-1:0] committed_wid;
    wire [`ISSUE_WIDTH-1:0] true_eop;

    modport master (
        output committed,
        output true_eop,
        output committed_wid
    );

    modport slave (
        input committed,
        input true_eop,
        input committed_wid
    );

endinterface
