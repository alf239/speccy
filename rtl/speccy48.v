// ---------------------------------------------------------------------------
// speccy48 -- the complete 48K machine: TV80 in the socket of speccy.v
//
// This is the module a top level (board or simulation) instantiates. The bus
// is internal; what remains are the peripherals and the video output.
// ---------------------------------------------------------------------------

`default_nettype none

module speccy48 #(
    parameter ROM_FILE   = "",
    parameter VRAM_FILE  = "",
    parameter RAM_FILE   = "",
    parameter STUB_FILE  = "",
    parameter SNAP_FILE  = "",
    parameter DIVMMC_ROM = ""
)(
    input  wire        clk,          // 14 MHz
    input  wire        rst,
    input  wire        arm_snapshot, // sampled at reset; tie low for normal boot
    input  wire        divmmc_en,    // runtime divMMC enable
    input  wire        nmi_button,   // active high; one press = one NMI

    output wire        sd_cs,
    output wire        sd_sck,
    output wire        sd_mosi,
    input  wire        sd_miso,
    output wire [15:0] dbg_sd,      // SD diagnostics: {exchange ctr, last rx}

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
    wire        boot_busy;
    wire        cpu_wait_n;
    wire [15:0] cpu_a;

    // NMI: synchronise the button, one fixed-width low pulse per press. The
    // Z80 latches the falling edge; esxDOS's 0x66 trap does the rest.
    reg [2:0]  nmi_sync;
    reg [10:0] nmi_ctr;
    always @(posedge clk) begin
        nmi_sync <= {nmi_sync[1:0], nmi_button};
        if (rst)                            nmi_ctr <= 11'd0;
        else if (nmi_sync[1] && !nmi_sync[2]) nmi_ctr <= 11'd1400;  // ~100 us
        else if (nmi_ctr != 11'd0)          nmi_ctr <= nmi_ctr - 11'd1;
    end
    wire nmi_n = (nmi_ctr == 11'd0);
    wire [7:0]  cpu_do, cpu_di;
    wire        mreq_n, iorq_n, rd_n, wr_n, m1_n, int_n;

    // T2Write=1: wr_n asserts in T2, matching a real Z80 closely enough for
    // synchronous RAM. IOWait=1: standard 4-T-state I/O cycle with the extra
    // wait T-state, which the ULA port timing assumes.
    tv80s #(.Mode(0), .T2Write(1), .IOWait(1)) u_cpu (
        .clk     (clk),
        .cen     (ce_cpu),
        // Held in reset while the boot copier refills RAM from the snapshot
        // shadow (~7 ms); the video side keeps running.
        .reset_n (~(rst | boot_busy)),
        .wait_n  (cpu_wait_n),  // stretched only by in-flight SPI exchanges
        .int_n   (int_n),
        .nmi_n   (nmi_n),
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

    speccy #(.ROM_FILE(ROM_FILE), .VRAM_FILE(VRAM_FILE),
             .RAM_FILE(RAM_FILE), .STUB_FILE(STUB_FILE),
             .SNAP_FILE(SNAP_FILE), .DIVMMC_ROM(DIVMMC_ROM)) u_machine (
        .clk          (clk),
        .rst          (rst),
        .arm_snapshot (arm_snapshot),
        .divmmc_en    (divmmc_en),
        .boot_busy    (boot_busy),
        .cpu_wait_n   (cpu_wait_n),
        .sd_cs        (sd_cs),
        .sd_sck       (sd_sck),
        .sd_mosi      (sd_mosi),
        .sd_miso      (sd_miso),
        .dbg_sd       (dbg_sd),
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
