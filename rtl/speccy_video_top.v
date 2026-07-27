// ---------------------------------------------------------------------------
// speccy_video_top -- stage 0/1 top level
//
// Video generator + screen bank, with the CPU-side RAM port brought out so a
// testbench (or later, a real Z80) can write the screen. No CPU yet.
//
// Clock is 14 MHz. On hardware that comes from a PLL: 50 x 7/25 = 14 MHz
// exactly, which is why the DE10-Lite's 50 MHz oscillator suits a Spectrum
// without any fractional-N awkwardness.
// ---------------------------------------------------------------------------

`default_nettype none

module speccy_video_top (
    input  wire        clk,            // 14 MHz
    input  wire        rst,

    input  wire [2:0]  border_colour,

    // CPU-side port into the screen bank
    input  wire [13:0] cpu_addr,
    input  wire [7:0]  cpu_din,
    input  wire        cpu_we,
    output wire [7:0]  cpu_dout,

    output wire [3:0]  vga_r,
    output wire [3:0]  vga_g,
    output wire [3:0]  vga_b,
    output wire        vga_hsync,
    output wire        vga_vsync,
    output wire        vga_blank,

    output wire        ce_pix,
    output wire [8:0]  px_h,
    output wire [8:0]  px_v,
    output wire        frame_end
);

    // 14 MHz -> 7 MHz pixel enable
    reg ce_pix_r;
    always @(posedge clk) begin
        if (rst) ce_pix_r <= 1'b0;
        else     ce_pix_r <= ~ce_pix_r;
    end
    assign ce_pix = ce_pix_r;

    wire [13:0] vram_addr;
    wire [7:0]  vram_data;

    vram #(.ADDR_W(14)) u_vram (
        .clk    (clk),
        .a_addr (cpu_addr),
        .a_din  (cpu_din),
        .a_we   (cpu_we),
        .a_dout (cpu_dout),
        .b_addr (vram_addr),
        .b_dout (vram_data)
    );

    video u_video (
        .clk           (clk),
        .rst           (rst),
        .ce_pix        (ce_pix),
        .border_colour (border_colour),
        .vram_addr     (vram_addr),
        .vram_data     (vram_data),
        .vga_r         (vga_r),
        .vga_g         (vga_g),
        .vga_b         (vga_b),
        .vga_hsync     (vga_hsync),
        .vga_vsync     (vga_vsync),
        .vga_blank     (vga_blank),
        .px_h          (px_h),
        .px_v          (px_v),
        .frame_end     (frame_end)
    );

endmodule

`default_nettype wire
