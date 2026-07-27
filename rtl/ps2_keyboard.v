// ---------------------------------------------------------------------------
// ps2_keyboard -- scancode set 2 to the Spectrum's 40-key matrix
//
// Tracks E0 (extended) and F0 (break) prefixes, maps each key to one or two
// matrix positions, and maintains the pressed-state of all 40 keys. Two
// positions because some PC keys are Spectrum chords: Backspace is
// CAPS SHIFT + 0, the arrows are CAPS SHIFT + 5/6/7/8.
//
// Matrix bit numbering matches keyboard.v: bit = row*5 + key, rows selected
// by A8..A15:
//
//   row 0 (A8):  CAPS  Z  X  C  V      row 4 (A12): 0  9  8  7  6
//   row 1 (A9):  A  S  D  F  G         row 5 (A13): P  O  I  U  Y
//   row 2 (A10): Q  W  E  R  T         row 6 (A14): ENTER  L  K  J  H
//   row 3 (A11): 1  2  3  4  5         row 7 (A15): SPACE  SYM  M  N  B
//
// Mapping choices (phase 1):
//   PC Shift (either)  -> CAPS SHIFT        PC Ctrl (either) -> SYMBOL SHIFT
//   Backspace          -> CAPS + 0          Arrows -> CAPS + 5/6/7/8
//   ' (quote key)      -> SYM + P  (")      , -> SYM + N   . -> SYM + M
//   - -> SYM + J       = -> SYM + L         ; -> SYM + O   / -> SYM + V
//
// Known simplification: the punctuation mappings ignore the PC shift state,
// so Shift+' (which produces " on a PC layout) and plain ' both land on
// SYM+P. A full layout-aware remap that tracks shift and untangles chords is
// phase 2 polish; this is enough to type BASIC.
//
// E0 12 / E0 F0 12 are the "fake shifts" some keyboards wrap around arrow
// and PrtSc codes -- explicitly ignored, or arrows would ghost a CAPS press.
// ---------------------------------------------------------------------------

`default_nettype none

module ps2_keyboard (
    input  wire        clk,
    input  wire        rst,

    input  wire [7:0]  code,
    input  wire        valid,

    output reg  [39:0] key_matrix      // active high, 1 = pressed
);

    reg e0, f0;

    // Decode one scancode to up to two matrix positions.
    reg       hit1, hit2;
    reg [5:0] pos1, pos2;

    localparam CAPS = 6'd0,  SYM = 6'd36;

    always @* begin
        hit1 = 1'b1;                   // most codes map to at least one key
        hit2 = 1'b0;
        pos1 = 6'd0;
        pos2 = 6'd0;

        if (e0) begin
            case (code)                // extended codes: arrows, right Ctrl
                8'h6B: begin pos1 = CAPS; hit2 = 1'b1; pos2 = 6'd19; end // left  = CAPS+5
                8'h72: begin pos1 = CAPS; hit2 = 1'b1; pos2 = 6'd24; end // down  = CAPS+6
                8'h75: begin pos1 = CAPS; hit2 = 1'b1; pos2 = 6'd23; end // up    = CAPS+7
                8'h74: begin pos1 = CAPS; hit2 = 1'b1; pos2 = 6'd22; end // right = CAPS+8
                8'h14: pos1 = SYM;                                       // right Ctrl
                default: hit1 = 1'b0;  // includes E0 12 fake shift
            endcase
        end else begin
            case (code)
                // row 0
                8'h12, 8'h59: pos1 = CAPS;              // either PC shift
                8'h1A: pos1 = 6'd1;   8'h22: pos1 = 6'd2;   // Z X
                8'h21: pos1 = 6'd3;   8'h2A: pos1 = 6'd4;   // C V
                // row 1
                8'h1C: pos1 = 6'd5;   8'h1B: pos1 = 6'd6;   // A S
                8'h23: pos1 = 6'd7;   8'h2B: pos1 = 6'd8;   // D F
                8'h34: pos1 = 6'd9;                         // G
                // row 2
                8'h15: pos1 = 6'd10;  8'h1D: pos1 = 6'd11;  // Q W
                8'h24: pos1 = 6'd12;  8'h2D: pos1 = 6'd13;  // E R
                8'h2C: pos1 = 6'd14;                        // T
                // row 3
                8'h16: pos1 = 6'd15;  8'h1E: pos1 = 6'd16;  // 1 2
                8'h26: pos1 = 6'd17;  8'h25: pos1 = 6'd18;  // 3 4
                8'h2E: pos1 = 6'd19;                        // 5
                // row 4
                8'h45: pos1 = 6'd20;  8'h46: pos1 = 6'd21;  // 0 9
                8'h3E: pos1 = 6'd22;  8'h3D: pos1 = 6'd23;  // 8 7
                8'h36: pos1 = 6'd24;                        // 6
                // row 5
                8'h4D: pos1 = 6'd25;  8'h44: pos1 = 6'd26;  // P O
                8'h43: pos1 = 6'd27;  8'h3C: pos1 = 6'd28;  // I U
                8'h35: pos1 = 6'd29;                        // Y
                // row 6
                8'h5A: pos1 = 6'd30;  8'h4B: pos1 = 6'd31;  // Enter L
                8'h42: pos1 = 6'd32;  8'h3B: pos1 = 6'd33;  // K J
                8'h33: pos1 = 6'd34;                        // H
                // row 7
                8'h29: pos1 = 6'd35;  8'h14: pos1 = SYM;    // Space, left Ctrl
                8'h3A: pos1 = 6'd37;  8'h31: pos1 = 6'd38;  // M N
                8'h32: pos1 = 6'd39;                        // B
                // chords
                8'h66: begin pos1 = CAPS; hit2 = 1'b1; pos2 = 6'd20; end // Backspace = CAPS+0
                8'h52: begin pos1 = SYM;  hit2 = 1'b1; pos2 = 6'd25; end // '  -> SYM+P  (")
                8'h41: begin pos1 = SYM;  hit2 = 1'b1; pos2 = 6'd38; end // ,  -> SYM+N
                8'h49: begin pos1 = SYM;  hit2 = 1'b1; pos2 = 6'd37; end // .  -> SYM+M
                8'h4E: begin pos1 = SYM;  hit2 = 1'b1; pos2 = 6'd33; end // -  -> SYM+J
                8'h55: begin pos1 = SYM;  hit2 = 1'b1; pos2 = 6'd31; end // =  -> SYM+L
                8'h4C: begin pos1 = SYM;  hit2 = 1'b1; pos2 = 6'd26; end // ;  -> SYM+O
                8'h4A: begin pos1 = SYM;  hit2 = 1'b1; pos2 = 6'd4;  end // /  -> SYM+V
                default: hit1 = 1'b0;
            endcase
        end
    end

    always @(posedge clk) begin
        if (rst) begin
            e0 <= 1'b0;
            f0 <= 1'b0;
            key_matrix <= 40'd0;
        end else if (valid) begin
            if (code == 8'hE0)      e0 <= 1'b1;
            else if (code == 8'hF0) f0 <= 1'b1;      // e0 kept: E0 F0 xx order
            else begin
                if (hit1) key_matrix[pos1] <= ~f0;
                if (hit2) key_matrix[pos2] <= ~f0;
                e0 <= 1'b0;
                f0 <= 1'b0;
            end
        end
    end

endmodule

`default_nettype wire
