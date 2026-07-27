// ---------------------------------------------------------------------------
// palette -- 4-bit Spectrum colour index to 4:4:4 RGB
//
// Index is { BRIGHT, green, red, blue } -- the ordering of the low three bits
// is the Spectrum's own, and the reason colour 1 is blue rather than red.
//
// Keeping the pipeline indexed all the way to the VGA pins means the
// scandoubler's line buffer stores 4 bits per pixel instead of 12, and leaves
// one obvious place to hook a programmable palette later (ULAplus and friends).
// ---------------------------------------------------------------------------

`default_nettype none

module palette #(
    // The normal/bright ratio is a judgement call; real Spectrums vary between
    // ULA revisions anyway. Tune against a monitor at stage 1.
    parameter [3:0] LEVEL_NORMAL = 4'hC,
    parameter [3:0] LEVEL_BRIGHT = 4'hF
)(
    input  wire [3:0] idx,
    input  wire       blank,
    output wire [3:0] r,
    output wire [3:0] g,
    output wire [3:0] b
);

    wire [3:0] level = idx[3] ? LEVEL_BRIGHT : LEVEL_NORMAL;

    assign r = (blank || !idx[1]) ? 4'h0 : level;
    assign g = (blank || !idx[2]) ? 4'h0 : level;
    assign b = (blank || !idx[0]) ? 4'h0 : level;

endmodule

`default_nettype wire
