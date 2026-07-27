// ---------------------------------------------------------------------------
// video -- ZX Spectrum video generator (the display half of a ULA)
//
// Produces 4-bit-per-channel RGB for the DE10-Lite's VGA DAC, plus sync.
//
// Screen memory layout (addresses relative to the start of the 16 KB bank):
//
//   bitmap    0x0000..0x17FF   6144 bytes
//     address = { y[7:6], y[2:0], y[5:3], x[4:0] }
//     -- the famous non-linear layout: third of the screen, then pixel row
//        within the character, then character row within the third.
//
//   attribute 0x1800..0x1AFF    768 bytes
//     address = 0x1800 | (y[7:3] << 5) | x[4:0]
//     bit 7 FLASH, bit 6 BRIGHT, bits 5:3 PAPER, bits 2:0 INK
//     colour bits are { green, red, blue }
//
// Fetch scheduling: each character cell is 8 pixels = 16 clk cycles at 14 MHz,
// and we need two bytes per cell. We fetch one cell ahead, using four of the
// eight pixel slots:
//
//   pixel 4  present bitmap address
//   pixel 5  latch bitmap data      (RAM output is registered, 1 clk latency)
//   pixel 6  present attribute address
//   pixel 7  latch attribute, load shift register for the next cell
//
// That leaves the other RAM port entirely free for the CPU, which is the whole
// point -- no arbitration, no contention, video and CPU never collide.
//
// Fetching one cell ahead also wraps across the end of a line, which is how
// the first cell of each line gets fetched during the preceding left border.
// ---------------------------------------------------------------------------

`default_nettype none

module video (
    input  wire        clk,           // 14 MHz
    input  wire        rst,
    input  wire        ce_pix,        // 7 MHz enable

    input  wire [2:0]  border_colour, // port 0xFE bits 2:0

    // Video-side read port into the screen bank
    output reg  [13:0] vram_addr,
    input  wire [7:0]  vram_data,

    // Pixel stream out, as 4-bit colour indices { BRIGHT, green, red, blue }.
    // Expansion to RGB happens after the scandoubler, so the line buffer only
    // has to carry 4 bits per pixel.
    output reg  [3:0]  px_idx,
    output reg         px_hsync,
    output reg         px_vsync,
    output reg         px_blank,      // horizontal or vertical
    output reg         px_vblank,     // vertical only
    // Position of the pixel currently being emitted, pipelined to match px_idx
    // so a testbench can place it straight into a framebuffer.
    output reg  [8:0]  px_h,
    output reg  [8:0]  px_v,
    output wire        frame_end
);

    localparam [9:0] H_TOTAL = 10'd448;
    localparam [8:0] V_TOTAL = 9'd312;

    // -----------------------------------------------------------------------
    // Raster timing
    // -----------------------------------------------------------------------
    wire [8:0] hc, vc;
    wire       display, blank, v_blank, hsync, vsync;

    video_timing u_timing (
        .clk       (clk),
        .rst       (rst),
        .ce_pix    (ce_pix),
        .hc        (hc),
        .vc        (vc),
        .display   (display),
        .border    (),
        .blank     (blank),
        .v_blank   (v_blank),
        .hsync     (hsync),
        .vsync     (vsync),
        .frame_end (frame_end)
    );

    // -----------------------------------------------------------------------
    // Fetch position -- one character cell (8 pixels) ahead of the beam
    // -----------------------------------------------------------------------
    wire [9:0] fh_raw  = {1'b0, hc} + 10'd8;
    wire       fh_wrap = (fh_raw >= H_TOTAL);

    // When wrapping, fh_raw is 448..455, so the subtraction stays inside 9 bits.
    wire [8:0] fh = fh_wrap ? (fh_raw[8:0] - H_TOTAL[8:0]) : fh_raw[8:0];
    wire [8:0] fv = fh_wrap ? ((vc == V_TOTAL - 9'd1) ? 9'd0 : vc + 9'd1) : vc;

    wire       fetch_active = (fh < 9'd256) && (fv < 9'd192);

    wire [7:0] fy = fv[7:0];
    wire [4:0] fx = fh[7:3];

    wire [13:0] bitmap_addr = {1'b0,   fy[7:6], fy[2:0], fy[5:3], fx};
    wire [13:0] attr_addr   = {3'b011, 1'b0,    fy[7:3],          fx};  // 0x1800 | ...

    // -----------------------------------------------------------------------
    // Fetch and shift
    // -----------------------------------------------------------------------
    reg [7:0] bitmap_d;
    reg [7:0] shifter;
    reg [7:0] attr_cur;
    reg       fetch_act_l;   // latched when the address is issued, so that both
                             // latches use the state that applied at fetch time

    always @(posedge clk) begin
        if (rst) begin
            vram_addr   <= 14'd0;
            bitmap_d    <= 8'd0;
            shifter     <= 8'd0;
            attr_cur    <= 8'd0;
            fetch_act_l <= 1'b0;
        end else if (ce_pix) begin
            case (hc[2:0])
                3'd4: begin
                    vram_addr   <= bitmap_addr;
                    fetch_act_l <= fetch_active;
                end
                3'd5: bitmap_d  <= fetch_act_l ? vram_data : 8'h00;
                3'd6: vram_addr <= attr_addr;
                default: ;
            endcase

            if (hc[2:0] == 3'd7) begin
                // Load for the next cell. The pixel emitted this cycle still
                // comes from the current shifter value, sampled combinationally
                // below, so there is no conflict.
                shifter  <= bitmap_d;
                attr_cur <= fetch_act_l ? vram_data : 8'h00;
            end else begin
                shifter  <= {shifter[6:0], 1'b0};
            end
        end
    end

    // -----------------------------------------------------------------------
    // Flash: swaps ink and paper every 16 frames
    // -----------------------------------------------------------------------
    reg [4:0] flash_ctr;
    always @(posedge clk) begin
        if (rst)                      flash_ctr <= 5'd0;
        else if (ce_pix && frame_end) flash_ctr <= flash_ctr + 5'd1;
    end
    wire flash_state = flash_ctr[4];

    // -----------------------------------------------------------------------
    // Colour
    // -----------------------------------------------------------------------
    wire       pix   = shifter[7] ^ (attr_cur[7] & flash_state);
    wire [2:0] ink   = attr_cur[2:0];
    wire [2:0] paper = attr_cur[5:3];

    wire [2:0] colour = display ? (pix ? ink : paper) : border_colour;
    wire       bright = display ? attr_cur[6] : 1'b0;   // border is never bright

    always @(posedge clk) begin
        if (rst) begin
            px_idx    <= 4'd0;
            px_hsync  <= 1'b0;
            px_vsync  <= 1'b0;
            px_blank  <= 1'b1;
            px_vblank <= 1'b0;
            px_h      <= 9'd0;
            px_v      <= 9'd0;
        end else if (ce_pix) begin
            // Blanking is not forced into the index -- the line buffer carries
            // true colour and the palette blanks at the output instead.
            px_idx    <= {bright, colour};

            px_hsync  <= hsync;
            px_vsync  <= vsync;
            px_blank  <= blank;
            px_vblank <= v_blank;

            px_h      <= hc;
            px_v      <= vc;
        end
    end

endmodule

`default_nettype wire
