// Boot-test wrapper: the full machine with a ROM chosen at verilation time
// (-GROM_FILE=...). Unlike speccy48_tb_top there are no debug taps -- the
// harness watches only what a monitor would see, which is the point.

`default_nettype none

module boot_tb_top #(
    parameter ROM_FILE   = "sim/cpu_rom.hex",
    parameter VRAM_FILE  = "",
    parameter RAM_FILE   = "",
    parameter STUB_FILE  = "",
    parameter SNAP_FILE  = "",
    parameter DIVMMC_ROM = "",
    parameter ROM128_FILE = ""
)(
    input  wire        clk,
    input  wire        rst,
    input  wire        arm_snapshot,
    input  wire        divmmc_en,
    input  wire        en_128,
    input  wire        nmi_button,
    output wire        sd_cs,
    output wire        sd_sck,
    output wire        sd_mosi,
    input  wire        sd_miso,

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

    input  wire [39:0] key_matrix,   // OR-ed with the PS/2-derived matrix
    input  wire        ps2_clk,
    input  wire        ps2_data,
    input  wire [4:0]  joy_state,
    input  wire        ear_in,
    output wire        speaker,
    output wire [9:0]  audio,
    output wire        mic,
    output wire [2:0]  border,

    output wire [3:0]  vga_r,
    output wire [3:0]  vga_g,
    output wire [3:0]  vga_b,
    output wire        vga_hsync,
    output wire        vga_vsync,
    output wire        vga_blank
);

    // PS/2 chain, exactly as wired on the board
    wire [7:0]  ps2_code;
    wire        ps2_valid;
    wire [39:0] ps2_matrix;

    ps2_rx u_ps2_rx (
        .clk (clk), .rst (rst),
        .ps2_clk (ps2_clk), .ps2_data (ps2_data),
        .code (ps2_code), .valid (ps2_valid), .frame_err ()
    );

    ps2_keyboard u_ps2_map (
        .clk (clk), .rst (rst),
        .code (ps2_code), .valid (ps2_valid),
        .key_matrix (ps2_matrix)
    );

    speccy48 #(.ROM_FILE(ROM_FILE), .VRAM_FILE(VRAM_FILE),
               .RAM_FILE(RAM_FILE), .STUB_FILE(STUB_FILE),
               .SNAP_FILE(SNAP_FILE), .DIVMMC_ROM(DIVMMC_ROM),
               .ROM128_FILE(ROM128_FILE)) u_ss (
        .clk          (clk),
        .rst          (rst),
        .divmmc_en    (divmmc_en),
        .en_128       (en_128),
        .nmi_button   (nmi_button),
        .sd_cs (sd_cs), .sd_sck (sd_sck), .sd_mosi (sd_mosi),
        .sd_miso (sd_miso), .dbg_sd (),
        .dram_addr (dram_addr), .dram_ba (dram_ba),
        .dram_dq_in (dram_dq_in), .dram_dq_out (dram_dq_out),
        .dram_dq_oe (dram_dq_oe),
        .dram_ldqm (dram_ldqm), .dram_udqm (dram_udqm),
        .dram_ras_n (dram_ras_n), .dram_cas_n (dram_cas_n),
        .dram_we_n (dram_we_n), .dram_cs_n (dram_cs_n),
        .dram_cke (dram_cke),
        .arm_snapshot (arm_snapshot),
        .key_matrix (key_matrix | ps2_matrix),
        .joy_state  (joy_state),
        .ear_in     (ear_in),
        .ym_mode    (1'b0),
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
