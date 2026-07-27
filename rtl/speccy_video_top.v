// ---------------------------------------------------------------------------
// speccy_video_top -- stage 0/1 top level
//
//   video  --(7 MHz, 4-bit indices)-->  scandoubler  --(14 MHz)-->  palette
//
// Screen bank with the CPU-side RAM port brought out so a testbench (or later,
// a real Z80) can write the screen. No CPU yet.
//
// Clock is 14 MHz. On hardware that comes from a PLL: 50 x 7/25 = 14 MHz
// exactly, which is why the DE10-Lite's 50 MHz oscillator suits a Spectrum
// without any fractional-N awkwardness.
//
// Both sides of the scandoubler are brought out, so the testbench can capture
// the 15.625 kHz input raster and the 31.25 kHz output raster and check one
// against the other.
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

    // VGA output -- 31.25 kHz, what the monitor sees
    output wire [3:0]  vga_r,
    output wire [3:0]  vga_g,
    output wire [3:0]  vga_b,
    output wire        vga_hsync,
    output wire        vga_vsync,
    output wire        vga_blank,
    output wire [8:0]  out_h,
    output wire [9:0]  out_v,

    // Pre-scandoubler stream -- 15.625 kHz, for verification
    output wire        ce_pix,
    output wire [3:0]  px_idx,
    output wire        px_blank,
    output wire [8:0]  px_h,
    output wire [8:0]  px_v,
    output wire        frame_end
);

    localparam [8:0] V_LAST = 9'd311;   // final line of the input frame

    // 14 MHz -> 7 MHz pixel enable
    reg ce_pix_r;
    always @(posedge clk) begin
        if (rst) ce_pix_r <= 1'b0;
        else     ce_pix_r <= ~ce_pix_r;
    end
    assign ce_pix = ce_pix_r;

    // -----------------------------------------------------------------------
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

    wire px_vsync, px_vblank;

    video u_video (
        .clk           (clk),
        .rst           (rst),
        .ce_pix        (ce_pix),
        .border_colour (border_colour),
        .vram_addr     (vram_addr),
        .vram_data     (vram_data),
        .px_idx        (px_idx),
        .px_hsync      (),                // scandoubler regenerates hsync
        .px_vsync      (px_vsync),
        .px_blank      (px_blank),
        .px_vblank     (px_vblank),
        .px_h          (px_h),
        .px_v          (px_v),
        .frame_end     (frame_end)
    );

    wire [3:0] out_idx;

    scandoubler u_scandoubler (
        .clk       (clk),
        .rst       (rst),
        .ce_pix    (ce_pix),
        .in_h      (px_h),
        .in_v      (px_v),
        .in_idx    (px_idx),
        .in_vsync  (px_vsync),
        .in_vblank (px_vblank),
        .in_v_last (V_LAST),
        .out_idx   (out_idx),
        .out_h     (out_h),
        .out_v     (out_v),
        .out_hsync (vga_hsync),
        .out_vsync (vga_vsync),
        .out_blank (vga_blank)
    );

    palette u_palette (
        .idx   (out_idx),
        .blank (vga_blank),
        .r     (vga_r),
        .g     (vga_g),
        .b     (vga_b)
    );

endmodule

`default_nettype wire
