// Boot-test wrapper: the full machine with a ROM chosen at verilation time
// (-GROM_FILE=...). Unlike speccy48_tb_top there are no debug taps -- the
// harness watches only what a monitor would see, which is the point.

`default_nettype none

module boot_tb_top #(
    parameter ROM_FILE = "sim/cpu_rom.hex"
)(
    input  wire        clk,
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

    speccy48 #(.ROM_FILE(ROM_FILE)) u_ss (
        .clk        (clk),
        .rst        (rst),
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
