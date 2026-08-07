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
    parameter DIVMMC_ROM = "",
    parameter ROM128_FILE = ""
)(
    input  wire        clk,          // 14 MHz
    input  wire        rst,
    input  wire        arm_snapshot, // sampled at reset; tie low for normal boot
    input  wire        divmmc_en,    // runtime divMMC enable
    input  wire        en_128,       // 128K mode request, sampled at reset
    input  wire        nmi_button,   // active high; one press = one NMI

    output wire        sd_cs,
    output wire        sd_sck,
    output wire        sd_mosi,
    input  wire        sd_miso,
    output wire [15:0] dbg_sd,      // SD diagnostics: {exchange ctr, last rx}

    output wire [12:0] dram_addr,
    output wire [1:0]  dram_ba,
    input  wire [15:0] dram_dq_in,
    output wire [15:0] dram_dq_out,
    output wire        dram_dq_oe,
    output wire        dram_ldqm,
    output wire        dram_udqm,
    output wire        dram_ras_n,
    output wire        dram_cas_n,
    output wire        dram_we_n,
    output wire        dram_cs_n,
    output wire        dram_cke,

    input  wire [39:0] key_matrix,
    input  wire [4:0]  joy_state,
    input  wire        ear_in,
    input  wire        ym_mode,     // AY/YM envelope-resolution switch
    output wire        speaker,
    output wire [9:0]  audio,      // beeper + AY mix for a sigma-delta DAC
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
             .SNAP_FILE(SNAP_FILE), .DIVMMC_ROM(DIVMMC_ROM),
             .ROM128_FILE(ROM128_FILE)) u_machine (
        .clk          (clk),
        .rst          (rst),
        .arm_snapshot (arm_snapshot),
        .divmmc_en    (divmmc_en),
        .en_128       (en_128),
        .boot_busy    (boot_busy),
        .cpu_wait_n   (cpu_wait_n),
        .sd_cs        (sd_cs),
        .sd_sck       (sd_sck),
        .sd_mosi      (sd_mosi),
        .sd_miso      (sd_miso),
        .dbg_sd       (dbg_sd),
        .dram_addr (dram_addr), .dram_ba (dram_ba),
        .dram_dq_in (dram_dq_in), .dram_dq_out (dram_dq_out),
        .dram_dq_oe (dram_dq_oe),
        .dram_ldqm (dram_ldqm), .dram_udqm (dram_udqm),
        .dram_ras_n (dram_ras_n), .dram_cas_n (dram_cas_n),
        .dram_we_n (dram_we_n), .dram_cs_n (dram_cs_n),
        .dram_cke (dram_cke),
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
        .ym_mode    (ym_mode),
        .speaker    (speaker),
        .audio      (audio),
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
