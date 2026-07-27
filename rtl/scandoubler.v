// ---------------------------------------------------------------------------
// scandoubler -- 15.625 kHz -> 31.25 kHz
//
// A Spectrum line is 448 pixels at 7 MHz = 64 us. Read the same 448 pixels back
// at 14 MHz and it takes 32 us, so each input line can be emitted twice in the
// time the next one arrives. Frame rate is untouched: 312 lines in, 624 out,
// still 50.08 Hz.
//
//   in : 448 x 312 @ 7 MHz   15.625 kHz line,  50.08 Hz frame
//   out: 448 x 624 @ 14 MHz  31.25  kHz line,  50.08 Hz frame
//
// 31.25 kHz is inside what a VGA monitor will lock to (640x480 is 31.469 kHz),
// which is the entire point of the exercise.
//
// Two line buffers, ping-ponged: line N is written while line N-1 is read out
// twice. No rate matching or FIFO logic is needed because the ratio is exactly
// 2:1 -- one input line is 896 cycles of the 14 MHz clock, which is precisely
// two output lines of 448. The output counter is simply reset at the start of
// each input line and free-runs.
//
// Horizontal positions are carried straight through: the output line has the
// same 448-position layout as the input, so sync and blanking are regenerated
// from the same numbers, just clocked twice as fast. The parameters below MUST
// match video_timing.v.
//
// Vertical sync and blanking are passed through rather than regenerated, but
// delayed by one input line so that they line up with the data being emitted
// (which is always one line behind what is arriving).
// ---------------------------------------------------------------------------

`default_nettype none

module scandoubler #(
    parameter [8:0] H_TOTAL    = 9'd448,
    parameter [8:0] H_BLANK_B  = 9'd304,
    parameter [8:0] H_BLANK_E  = 9'd400,
    parameter [8:0] H_SYNC_B   = 9'd320,
    parameter [8:0] H_SYNC_E   = 9'd352,
    parameter [9:0] V_TOTAL_X2 = 10'd624
)(
    input  wire       clk,        // 14 MHz
    input  wire       rst,
    input  wire       ce_pix,     // 7 MHz -- input side

    // Input stream. in_h/in_v accompany in_idx (same pipeline stage).
    input  wire [8:0] in_h,
    input  wire [8:0] in_v,
    input  wire [3:0] in_idx,
    input  wire       in_vsync,
    input  wire       in_vblank,
    input  wire [8:0] in_v_last,  // index of the final line of the frame

    // Doubled output, one pixel every clk
    output reg  [3:0] out_idx,
    output reg  [8:0] out_h,
    output reg  [9:0] out_v,
    output reg        out_hsync,
    output reg        out_vsync,
    output reg        out_blank
);

    // Two banks of 448, addressed as {bank, 9-bit position}. 1024 x 4 bits
    // fits in a single M9K with room to spare.
    reg [3:0] linebuf [0:1023];

    reg       wr_bank;
    reg       vsync_d;
    reg       vblank_d;

    // The final pixel of an input line: the next one starts a new line.
    wire line_start  = ce_pix && (in_h == H_TOTAL - 9'd1);
    wire frame_last  = (in_v == in_v_last);

    // Position about to be emitted.
    wire [8:0] next_h = line_start          ? 9'd0 :
                        (out_h == H_TOTAL - 9'd1) ? 9'd0 : out_h + 9'd1;

    // At the line boundary the bank being written is the one that just
    // completed, so that is what we start reading; otherwise read the other.
    wire rd_bank = line_start ? wr_bank : ~wr_bank;

    // Same lookahead for the vertical flags. line_start emits the first pixel
    // of the next output line, so it must already see the newly latched
    // values -- otherwise pixel 0 of every line carries the previous line's
    // blanking, which only shows up as a one-pixel defect at the two lines
    // where vblank actually changes.
    wire vsync_next  = line_start ? in_vsync  : vsync_d;
    wire vblank_next = line_start ? in_vblank : vblank_d;

    always @(posedge clk) begin
        if (rst) begin
            wr_bank   <= 1'b0;
            vsync_d   <= 1'b0;
            vblank_d  <= 1'b0;
            out_h     <= 9'd0;
            out_v     <= 10'd0;
            out_idx   <= 4'd0;
            out_hsync <= 1'b0;
            out_vsync <= 1'b0;
            out_blank <= 1'b1;
        end else begin
            // ---- write side (7 MHz) --------------------------------------
            // in_h is used directly as the address, so the buffer cannot drift
            // out of phase with the incoming pixels.
            if (ce_pix) linebuf[{wr_bank, in_h}] <= in_idx;

            if (line_start) begin
                wr_bank  <= ~wr_bank;
                vsync_d  <= in_vsync;
                vblank_d <= in_vblank;
            end

            // ---- read side (14 MHz) --------------------------------------
            out_idx <= linebuf[{rd_bank, next_h}];
            out_h   <= next_h;

            if (line_start)
                out_v <= frame_last ? 10'd0 : out_v + 10'd1;
            else if (out_h == H_TOTAL - 9'd1)
                out_v <= (out_v == V_TOTAL_X2 - 10'd1) ? 10'd0 : out_v + 10'd1;

            out_hsync <= (next_h >= H_SYNC_B)  && (next_h < H_SYNC_E);
            out_blank <= ((next_h >= H_BLANK_B) && (next_h < H_BLANK_E)) || vblank_next;
            out_vsync <= vsync_next;
        end
    end

endmodule

`default_nettype wire
