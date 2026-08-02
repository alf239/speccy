// ---------------------------------------------------------------------------
// video_timing -- ZX Spectrum raster timing generator
//
//   448 pixel clocks x 312 lines at 7 MHz
//     -> 15.625 kHz line rate, 50.08 Hz frame rate
//
// Horizontal, counting from the first displayed pixel:
//       0..255   display        (256)
//     256..303   right border   ( 48)
//     304..399   blanking       ( 96)   <- hsync sits inside this
//     400..447   left border    ( 48)
//
// Vertical, counting from the first displayed line:
//       0..191   display        (192)
//     192..247   bottom border  ( 56)
//     248..263   blanking       ( 16)   <- vsync sits inside this
//     264..311   top border     ( 48)
//
// The totals (448 x 312) and the display window (256 x 192) are the parts that
// must be right, and are. The split between blanking and border, and the exact
// sync position within the blanking region, are the least certain figures here
// -- tune them against a real monitor at stage 1 rather than trusting them now.
// They are parameters for exactly that reason.
// ---------------------------------------------------------------------------

`default_nettype none

module video_timing #(
    parameter H_DISPLAY   = 256,
    parameter H_RIGHT     = 48,
    parameter H_BLANK     = 96,
    parameter H_LEFT      = 48,

    parameter V_DISPLAY   = 192,
    parameter V_BOTTOM    = 56,
    parameter V_BLANK     = 16,
    parameter V_TOP       = 48,

    parameter H_SYNC_OFF  = 0,   // hsync start, measured into the blank region
    parameter H_SYNC_LEN  = 32,   // ~4.6 us at 7 MHz
    parameter V_SYNC_OFF  = 0,
    parameter V_SYNC_LEN  = 4
)(
    input  wire       clk,          // 14 MHz
    input  wire       rst,
    input  wire       ce_pix,       // 7 MHz enable

    output reg  [8:0] hc,           // 0..447
    output reg  [8:0] vc,           // 0..311

    output wire       display,      // inside the 256x192 window
    output wire       border,       // visible, but outside the window
    output wire       blank,        // h_blank | v_blank
    output wire       v_blank,      // vertical only -- the scandoubler
                                    // regenerates the horizontal part itself
    output wire       hsync,
    output wire       vsync,
    output wire       frame_end     // pulses on the final pixel of the frame
);

    localparam H_TOTAL      = H_DISPLAY + H_RIGHT + H_BLANK + H_LEFT;  // 448
    localparam V_TOTAL      = V_DISPLAY + V_BOTTOM + V_BLANK + V_TOP;  // 312

    localparam H_BLANK_BEG  = H_DISPLAY + H_RIGHT;                     // 304
    localparam H_BLANK_END  = H_BLANK_BEG + H_BLANK;                   // 400
    localparam V_BLANK_BEG  = V_DISPLAY + V_BOTTOM;                    // 248
    localparam V_BLANK_END  = V_BLANK_BEG + V_BLANK;                   // 264

    localparam H_SYNC_BEG   = H_BLANK_BEG + H_SYNC_OFF;
    localparam H_SYNC_END   = H_SYNC_BEG  + H_SYNC_LEN;
    localparam V_SYNC_BEG   = V_BLANK_BEG + V_SYNC_OFF;
    localparam V_SYNC_END   = V_SYNC_BEG  + V_SYNC_LEN;

    // -----------------------------------------------------------------------
    // Counters
    // -----------------------------------------------------------------------
    always @(posedge clk) begin
        if (rst) begin
            hc <= 9'd0;
            vc <= 9'd0;
        end else if (ce_pix) begin
            if (hc == H_TOTAL - 1) begin
                hc <= 9'd0;
                vc <= (vc == V_TOTAL - 1) ? 9'd0 : vc + 9'd1;
            end else begin
                hc <= hc + 9'd1;
            end
        end
    end

    // -----------------------------------------------------------------------
    // Region decode
    // -----------------------------------------------------------------------
    wire h_disp  = (hc < H_DISPLAY);
    wire v_disp  = (vc < V_DISPLAY);
    wire h_blank = (hc >= H_BLANK_BEG) && (hc < H_BLANK_END);

    assign v_blank   = (vc >= V_BLANK_BEG) && (vc < V_BLANK_END);
    assign display   = h_disp && v_disp;
    assign blank     = h_blank || v_blank;
    assign border    = !display && !blank;

    assign hsync     = (hc >= H_SYNC_BEG) && (hc < H_SYNC_END);
    assign vsync     = (vc >= V_SYNC_BEG) && (vc < V_SYNC_END);

    assign frame_end = (hc == H_TOTAL - 1) && (vc == V_TOTAL - 1);

endmodule

`default_nettype wire
