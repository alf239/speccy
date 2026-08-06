// ---------------------------------------------------------------------------
// ay8912 -- AY-3-8912 programmable sound generator, from the datasheet
//
// Three square-tone generators, one noise LFSR, one envelope generator, all
// derived from a master clock enable (1.75 MHz here, Pentagon convention).
// Datasheet dividers: tone and noise resolve at fM/16, the envelope steps at
// fM/(16*EP) so a full 16-step ramp repeats at fM/(256*EP).
//
// The ym_mode input selects the audible difference between the two chips
// people argue about: the AY-3-8910/12 envelope has 16 steps, the Yamaha
// YM2149 has 32. Internally the envelope index is always 5-bit; AY mode
// steps it by 2 at the fM/16 tick, YM mode by 1 at fM/8 -- same ramp rate,
// double the resolution. Fixed channel volumes map onto the odd entries of
// the shared 32-entry exponential DAC table (~1.19x per step, halving every
// four), which reproduces the AY's published sqrt(2)-per-level ladder.
//
// Register file per the datasheet, with unused bits masked at write so that
// reads return exactly what the chip would. R14 (I/O port A) stores and
// returns its value -- 128K detection routines probe it for readback.
//
// Bus interface: addr_wr latches the register number (a write to 0xFFFD),
// data_wr writes the selected register (0xBFFD), dout is the selected
// register's value (a read of 0xFFFD).
// ---------------------------------------------------------------------------

`default_nettype none

module ay8912 (
    input  wire       clk,
    input  wire       rst,
    input  wire       ce,          // 1.75 MHz master clock enable
    input  wire       ym_mode,     // 0 = AY 16-step envelope, 1 = YM 32-step

    input  wire       addr_wr,
    input  wire       data_wr,
    input  wire [7:0] din,
    output reg  [7:0] dout,

    output wire [9:0] audio        // three DAC'd channels, summed
);

    // -----------------------------------------------------------------------
    // Register file
    // -----------------------------------------------------------------------
    reg [3:0] rsel;
    reg [7:0] r[0:15];

    // Write masks: unused bits read back as 0, like the real chip.
    function [7:0] regmask(input [3:0] n);
        case (n)
            4'd1, 4'd3, 4'd5, 4'd13: regmask = 8'h0F;   // coarse periods, shape
            4'd6:                    regmask = 8'h1F;   // noise period
            4'd8, 4'd9, 4'd10:       regmask = 8'h1F;   // amplitude + M bit
            default:                 regmask = 8'hFF;
        endcase
    endfunction

    integer i;

    // -----------------------------------------------------------------------
    // Prescaler: tone/noise tick at fM/16, YM-mode envelope tick at fM/8
    // -----------------------------------------------------------------------
    reg [3:0] pre;
    wire tick16 = ce && (pre == 4'd15);
    wire tick8  = ce && (pre[2:0] == 3'd7);

    // -----------------------------------------------------------------------
    // Tone generators
    // -----------------------------------------------------------------------
    wire [11:0] per_a = {r[1][3:0], r[0]};
    wire [11:0] per_b = {r[3][3:0], r[2]};
    wire [11:0] per_c = {r[5][3:0], r[4]};

    reg [11:0] tcnt_a, tcnt_b, tcnt_c;
    reg        tone_a, tone_b, tone_c;

    // Period 0 behaves as 1 (the counter reloads immediately).
    wire [11:0] top_a = (per_a == 12'd0) ? 12'd1 : per_a;
    wire [11:0] top_b = (per_b == 12'd0) ? 12'd1 : per_b;
    wire [11:0] top_c = (per_c == 12'd0) ? 12'd1 : per_c;

    // -----------------------------------------------------------------------
    // Noise: 17-bit LFSR, taps 0 and 3, clocked at its own period
    // -----------------------------------------------------------------------
    wire [4:0] per_n = r[6][4:0];
    wire [4:0] top_n = (per_n == 5'd0) ? 5'd1 : per_n;
    reg  [4:0]  ncnt;
    reg  [16:0] lfsr;
    wire        noise = lfsr[0];

    // -----------------------------------------------------------------------
    // Envelope
    // -----------------------------------------------------------------------
    wire [15:0] per_e = {r[12], r[11]};
    wire [15:0] top_e = (per_e == 16'd0) ? 16'd1 : per_e;

    reg [15:0] ecnt;
    reg [4:0]  eidx;
    reg        e_attack;    // current ramp direction
    reg        e_stop;

    wire e_cont = r[13][3];
    wire e_hold = r[13][0];
    wire e_alt  = r[13][1];

    wire etick = ym_mode ? tick8 : tick16;
    wire [1:0] estep = ym_mode ? 2'd1 : 2'd2;

    // Next index, one bit wider so the boundary is visible.
    wire [5:0] eup   = {1'b0, eidx} + {4'd0, estep};
    wire [5:0] edown = {1'b0, eidx} - {4'd0, estep};

    always @(posedge clk) begin
        if (rst) begin
            rsel   <= 4'd0;
            for (i = 0; i < 16; i = i + 1) r[i] <= 8'd0;
            pre    <= 4'd0;
            tcnt_a <= 12'd0; tcnt_b <= 12'd0; tcnt_c <= 12'd0;
            tone_a <= 1'b0;  tone_b <= 1'b0;  tone_c <= 1'b0;
            ncnt   <= 5'd0;
            lfsr   <= 17'h1FFFF;
            ecnt   <= 16'd0;
            eidx   <= 5'd0;
            e_attack <= 1'b0;
            e_stop   <= 1'b1;
        end else begin
            if (addr_wr) rsel <= din[3:0];
            if (data_wr) begin
                r[rsel] <= din & regmask(rsel);
                if (rsel == 4'd13) begin
                    // Writing the shape restarts the envelope.
                    e_attack <= din[2];
                    eidx     <= din[2] ? 5'd0 : 5'd31;
                    ecnt     <= 16'd0;
                    e_stop   <= 1'b0;
                end
            end

            if (ce) pre <= pre + 4'd1;

            if (tick16) begin
                // Tones
                if (tcnt_a + 12'd1 >= top_a) begin tcnt_a <= 12'd0; tone_a <= ~tone_a; end
                else tcnt_a <= tcnt_a + 12'd1;
                if (tcnt_b + 12'd1 >= top_b) begin tcnt_b <= 12'd0; tone_b <= ~tone_b; end
                else tcnt_b <= tcnt_b + 12'd1;
                if (tcnt_c + 12'd1 >= top_c) begin tcnt_c <= 12'd0; tone_c <= ~tone_c; end
                else tcnt_c <= tcnt_c + 12'd1;

                // Noise
                if (ncnt + 5'd1 >= top_n) begin
                    ncnt <= 5'd0;
                    lfsr <= {lfsr[0] ^ lfsr[3], lfsr[16:1]};
                end else ncnt <= ncnt + 5'd1;
            end

            // Envelope (its own tick so YM mode can run at double step rate)
            if (etick && !e_stop) begin
                if (ecnt + 16'd1 >= top_e) begin
                    ecnt <= 16'd0;
                    if (e_attack ? eup[5] : edown[5]) begin
                        // Ramp finished
                        if (!e_cont) begin
                            eidx <= 5'd0; e_stop <= 1'b1;
                        end else if (e_hold) begin
                            eidx <= (e_attack ^ e_alt) ? 5'd31 : 5'd0;
                            e_stop <= 1'b1;
                        end else if (e_alt) begin
                            e_attack <= ~e_attack;
                            eidx <= e_attack ? 5'd31 : 5'd0;   // turn around
                        end else begin
                            eidx <= e_attack ? 5'd0 : 5'd31;   // saw wraps
                        end
                    end else begin
                        eidx <= e_attack ? eup[4:0] : edown[4:0];
                    end
                end else ecnt <= ecnt + 16'd1;
            end
        end
    end

    // -----------------------------------------------------------------------
    // Mixer: a channel sounds when every enabled source is high (enables are
    // active low, a disabled source reads as high -- straight off the sheet)
    // -----------------------------------------------------------------------
    wire gate_a = (r[7][0] | tone_a) & (r[7][3] | noise);
    wire gate_b = (r[7][1] | tone_b) & (r[7][4] | noise);
    wire gate_c = (r[7][2] | tone_c) & (r[7][5] | noise);

    // Level index: envelope when the M bit is set, else the fixed volume on
    // the odd rungs of the 32-entry ladder.
    wire [4:0] lvl_a = r[8][4]  ? eidx : {r[8][3:0],  1'b1};
    wire [4:0] lvl_b = r[9][4]  ? eidx : {r[9][3:0],  1'b1};
    wire [4:0] lvl_c = r[10][4] ? eidx : {r[10][3:0], 1'b1};

    // 32-entry exponential DAC: halves every four steps; the two lowest
    // entries are forced to zero so volume 0 is true silence.
    function [7:0] dac(input [4:0] v);
        case (v)
            5'd0:  dac = 8'd0;   5'd1:  dac = 8'd0;   5'd2:  dac = 8'd1;
            5'd3:  dac = 8'd2;   5'd4:  dac = 8'd2;   5'd5:  dac = 8'd3;
            5'd6:  dac = 8'd3;   5'd7:  dac = 8'd4;   5'd8:  dac = 8'd5;
            5'd9:  dac = 8'd6;   5'd10: dac = 8'd7;   5'd11: dac = 8'd8;
            5'd12: dac = 8'd10;  5'd13: dac = 8'd11;  5'd14: dac = 8'd13;
            5'd15: dac = 8'd16;  5'd16: dac = 8'd19;  5'd17: dac = 8'd22;
            5'd18: dac = 8'd27;  5'd19: dac = 8'd32;  5'd20: dac = 8'd38;
            5'd21: dac = 8'd45;  5'd22: dac = 8'd54;  5'd23: dac = 8'd64;
            5'd24: dac = 8'd76;  5'd25: dac = 8'd90;  5'd26: dac = 8'd107;
            5'd27: dac = 8'd128; 5'd28: dac = 8'd152; 5'd29: dac = 8'd180;
            5'd30: dac = 8'd214; default: dac = 8'd255;
        endcase
    endfunction

    wire [7:0] out_a = gate_a ? dac(lvl_a) : 8'd0;
    wire [7:0] out_b = gate_b ? dac(lvl_b) : 8'd0;
    wire [7:0] out_c = gate_c ? dac(lvl_c) : 8'd0;

    assign audio = {2'd0, out_a} + {2'd0, out_b} + {2'd0, out_c};

    always @(*) dout = r[rsel];

endmodule

`default_nettype wire
