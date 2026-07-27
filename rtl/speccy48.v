// ---------------------------------------------------------------------------
// speccy48 -- the complete 48K machine: TV80 in the socket of speccy.v
//
// This is the module a top level (board or simulation) instantiates. The bus
// is internal; what remains are the peripherals and the video output.
// ---------------------------------------------------------------------------

`default_nettype none

module speccy48 #(
    parameter ROM_FILE  = "",
    parameter VRAM_FILE = ""
)(
    input  wire        clk,          // 14 MHz
    input  wire        rst,

    input  wire [39:0] key_matrix,
    input  wire [4:0]  joy_state,
    input  wire        ear_in,
    output wire        speaker,
    output wire        mic,
    output wire [2:0]  border,

    output wire [3:0]  vga_r,
    output wire [3:0]  vga_g,
    output wire [3:0]  vga_b,
    output wire        vga_hsync,
    output wire        vga_vsync,
    output wire        vga_blank
);

    wire        ce_cpu;
    wire [15:0] cpu_a;
    wire [7:0]  cpu_do, cpu_di;
    wire        mreq_n, iorq_n, rd_n, wr_n, m1_n, int_n;

    // T2Write=1: wr_n asserts in T2, matching a real Z80 closely enough for
    // synchronous RAM. IOWait=1: standard 4-T-state I/O cycle with the extra
    // wait T-state, which the ULA port timing assumes.
    tv80s #(.Mode(0), .T2Write(1), .IOWait(1)) u_cpu (
        .clk     (clk),
        .cen     (ce_cpu),
        .reset_n (~rst),
        .wait_n  (1'b1),      // Pentagon timing: nothing ever inserts waits
        .int_n   (int_n),
        .nmi_n   (1'b1),
        .busrq_n (1'b1),
        .m1_n    (m1_n),
        .mreq_n  (mreq_n),
        .iorq_n  (iorq_n),
        .rd_n    (rd_n),
        .wr_n    (wr_n),
        .rfsh_n  (),
        .halt_n  (),
        .busak_n (),
        .A       (cpu_a),
        .di      (cpu_di),
        .dout    (cpu_do)
    );

    speccy #(.ROM_FILE(ROM_FILE), .VRAM_FILE(VRAM_FILE)) u_machine (
        .clk        (clk),
        .rst        (rst),
        .ce_cpu     (ce_cpu),
        .ce_pix     (),
        .cpu_a      (cpu_a),
        .cpu_do     (cpu_do),
        .cpu_di     (cpu_di),
        .mreq_n     (mreq_n),
        .iorq_n     (iorq_n),
        .rd_n       (rd_n),
        .wr_n       (wr_n),
        .m1_n       (m1_n),
        .int_n      (int_n),
        .key_matrix (key_matrix),
        .joy_state  (joy_state),
        .ear_in     (ear_in),
        .speaker    (speaker),
        .mic        (mic),
        .border     (border),
        .vga_r      (vga_r),
        .vga_g      (vga_g),
        .vga_b      (vga_b),
        .vga_hsync  (vga_hsync),
        .vga_vsync  (vga_vsync),
        .vga_blank  (vga_blank)
    );

endmodule

`default_nettype wire
