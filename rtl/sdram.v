// ---------------------------------------------------------------------------
// sdram -- byte-access controller for the DE10-Lite's IS42S16320 (32M x 16)
//
// Deliberately unheroic: the whole design lives in the one 14 MHz domain,
// where a clock period is 71 ns and every SDRAM timing parameter (tRP 20 ns,
// tRCD 20 ns, tRFC 70 ns) fits in one or two cycles. A full random access --
// ACTIVE, tRCD, READ with auto-precharge, CL=2, latch -- takes 6 cycles,
// ~430 ns, comfortably inside a Z80 memory cycle at 3.5 MHz. No wait states,
// Pentagon-style.
//
// Byte access rides the DQM lanes: byte address N is word N>>1 with
// LDQM/UDQM selecting the half. Reads drive both lanes low and the low
// address bit picks the byte from the returned word.
//
// Refresh: 8192 rows per 64 ms needs one AUTO REFRESH every 7.8 us; one is
// scheduled every 100 cycles (7.1 us) and taken whenever the controller is
// idle. Auto-precharge on every access means banks are always closed when
// refresh wants them.
//
// The DQ bus is split (dq_in / dq_out / dq_oe) so the module verilates
// cleanly; the board top owns the actual tristate assign. DRAM_CLK is the
// system clock inverted at the top level -- commands change on our rising
// edge, the chip samples half a period later, 35 ns of margin each way.
//
// Interface: pulse req with addr/din/we held; busy rises and falls when
// done; on reads, valid pulses with dout for one cycle. Requests issued
// during a refresh are latched and served right after it.
// ---------------------------------------------------------------------------

`default_nettype none

module sdram (
    input  wire        clk,          // 14 MHz
    input  wire        rst,

    input  wire [24:0] addr,         // byte address
    input  wire [7:0]  din,
    input  wire        we,
    input  wire        req,          // one-cycle pulse
    output reg  [7:0]  dout,
    output reg         valid,        // dout is fresh (reads only)
    output reg         busy,
    output reg         ready,        // init sequence finished

    output reg  [12:0] dram_addr,
    output reg  [1:0]  dram_ba,
    input  wire [15:0] dq_in,
    output reg  [15:0] dq_out,
    output reg         dq_oe,
    output reg         dram_ldqm,
    output reg         dram_udqm,
    output reg         dram_ras_n,
    output reg         dram_cas_n,
    output reg         dram_we_n,
    output wire        dram_cs_n,
    output wire        dram_cke
);

    assign dram_cs_n = 1'b0;
    assign dram_cke  = 1'b1;

    // Commands as {ras_n, cas_n, we_n}
    localparam CMD_NOP      = 3'b111;
    localparam CMD_ACTIVE   = 3'b011;
    localparam CMD_READ     = 3'b101;
    localparam CMD_WRITE    = 3'b100;
    localparam CMD_PRECHG   = 3'b010;
    localparam CMD_REFRESH  = 3'b001;
    localparam CMD_LOADMODE = 3'b000;

    task cmd(input [2:0] c);
        begin
            dram_ras_n <= c[2];
            dram_cas_n <= c[1];
            dram_we_n  <= c[0];
        end
    endtask

    // Mode register: CL=2, sequential, burst length 1, standard write
    localparam MODE = 13'b000_0_00_010_0_000;

    localparam S_INIT_WAIT = 4'd0,  S_INIT_PRE  = 4'd1, S_INIT_REF = 4'd2,
               S_INIT_MODE = 4'd3,  S_IDLE      = 4'd4,
               S_RCD       = 4'd6,  S_RW        = 4'd7, S_CL1      = 4'd8,
               S_CL2       = 4'd9,  S_LATCH     = 4'd10, S_COOL    = 4'd11,
               S_REF_GAP   = 4'd12;

    reg [3:0]  state;
    // 2 ms of settle at 14 MHz; the datasheet asks for 100-200 us.
    reg [14:0] wait_ctr;
    reg [3:0]  init_refs;
    reg [6:0]  ref_ctr;              // one refresh due per 100 cycles
    reg        ref_due;

    // Latched request (a req can land during a refresh)
    reg        pend, pend_we;
    reg [24:0] pend_addr;
    reg [7:0]  pend_din;

    always @(posedge clk) begin
        if (rst) begin
            state     <= S_INIT_WAIT;
            wait_ctr  <= 15'd28000;
            init_refs <= 4'd8;
            ref_ctr   <= 7'd0;
            ref_due   <= 1'b0;
            ready     <= 1'b0;
            pend      <= 1'b0;
            pend_we   <= 1'b0;
            pend_addr <= 25'd0;
            pend_din  <= 8'd0;
            busy      <= 1'b0;
            valid     <= 1'b0;
            dout      <= 8'd0;
            dq_oe     <= 1'b0;
            dq_out    <= 16'd0;
            dram_addr <= 13'd0;
            dram_ba   <= 2'd0;
            dram_ldqm <= 1'b1;
            dram_udqm <= 1'b1;
            cmd(CMD_NOP);
        end else begin
            valid <= 1'b0;
            cmd(CMD_NOP);
            dq_oe <= 1'b0;

            // Refresh bookkeeping runs regardless of state.
            if (ref_ctr == 7'd99) begin
                ref_ctr <= 7'd0;
                ref_due <= 1'b1;
            end else ref_ctr <= ref_ctr + 7'd1;

            // Accept a request any time after init; it waits its turn.
            if (req && ready) begin
                pend      <= 1'b1;
                pend_addr <= addr;
                pend_din  <= din;
                pend_we   <= we;
                busy      <= 1'b1;
            end

            case (state)
                S_INIT_WAIT: begin
                    if (wait_ctr == 15'd0) state <= S_INIT_PRE;
                    else wait_ctr <= wait_ctr - 15'd1;
                end
                S_INIT_PRE: begin
                    cmd(CMD_PRECHG);
                    dram_addr[10] <= 1'b1;        // all banks
                    state <= S_REF_GAP;           // tRP, then refreshes
                end
                S_INIT_REF: begin
                    if (init_refs != 4'd0) begin
                        cmd(CMD_REFRESH);
                        init_refs <= init_refs - 4'd1;
                        state <= S_REF_GAP;       // tRFC spacer
                    end else state <= S_INIT_MODE;
                end
                S_INIT_MODE: begin
                    cmd(CMD_LOADMODE);
                    dram_ba   <= 2'd0;
                    dram_addr <= MODE;
                    ready     <= 1'b1;
                    state <= S_COOL;              // tMRD, then idle
                end

                S_IDLE: begin
                    if (ref_due) begin
                        cmd(CMD_REFRESH);
                        ref_due <= 1'b0;
                        state <= S_REF_GAP;
                    end else if (pend) begin
                        cmd(CMD_ACTIVE);
                        dram_ba   <= {1'b0, pend_addr[24]};
                        dram_addr <= pend_addr[23:11];
                        state <= S_RCD;
                    end
                end

                S_RCD: state <= S_RW;             // tRCD: one NOP at 71 ns

                S_RW: begin
                    dram_ba        <= {1'b0, pend_addr[24]};
                    dram_addr      <= {2'd0, 1'b1, pend_addr[10:1]};  // A10: auto-precharge
                    if (pend_we) begin
                        cmd(CMD_WRITE);
                        dq_out    <= {pend_din, pend_din};
                        dq_oe     <= 1'b1;
                        dram_ldqm <= pend_addr[0];
                        dram_udqm <= ~pend_addr[0];
                        state <= S_COOL;          // tDAL before next ACTIVE
                    end else begin
                        cmd(CMD_READ);
                        dram_ldqm <= 1'b0;
                        dram_udqm <= 1'b0;
                        state <= S_CL1;
                    end
                end

                S_CL1:  state <= S_CL2;
                S_CL2:  state <= S_LATCH;
                S_LATCH: begin
                    dout  <= pend_addr[0] ? dq_in[15:8] : dq_in[7:0];
                    valid <= 1'b1;
                    pend  <= 1'b0;
                    busy  <= 1'b0;
                    dram_ldqm <= 1'b1;
                    dram_udqm <= 1'b1;
                    state <= S_IDLE;
                end

                S_COOL: begin                     // write / mode settle cycle
                    dram_ldqm <= 1'b1;
                    dram_udqm <= 1'b1;
                    if (pend && pend_we) begin
                        pend <= 1'b0;
                        busy <= 1'b0;
                    end
                    state <= S_IDLE;
                end

                // One spacer cycle after PRECHARGE-ALL or AUTO REFRESH;
                // routes back into the init chain until it completes.
                S_REF_GAP: state <= ready ? S_IDLE : S_INIT_REF;

                default: state <= S_IDLE;
            endcase
        end
    end

endmodule

`default_nettype wire
