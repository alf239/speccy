// ---------------------------------------------------------------------------
// de10_lite_speccy48 -- the full machine on the board
//
// Quartus only (vendor PLL). Boots the ROM in rom48.hex; with the real 48K
// ROM that means BASIC to the (c) 1982 message with nothing plugged in.
//
//   SW[9]      sync polarity (flip if the monitor won't lock)
//   KEY[0]     reset (hold)
//   LEDR[4:0]  joystick     LEDR[7:5] border     LEDR[8] PLL lock
//   LEDR[9]    heartbeat    HEX1/HEX0 Kempston port byte
//   GPIO[4:0]  joystick in  GPIO[35]  beeper out (RC filter -> 3.5mm jack)
//   GPIO[26]   PS/2 clock   GPIO[27]  PS/2 data   (header pins 31/32; wire
//              each through a 2.2k series resistor -- the keyboard is a 5V
//              device and MAX 10 is not 5V tolerant; the resistor limits the
//              clamp-diode current to a safe fraction of a milliamp. Power
//              the keyboard from the header's 5V pin, not 3.3V.)
// ---------------------------------------------------------------------------

`default_nettype none

module de10_lite_speccy48 (
    input  wire        MAX10_CLK1_50,

    input  wire [1:0]  KEY,          // active low
    input  wire [9:0]  SW,
    output wire [9:0]  LEDR,

    output wire [7:0]  HEX0,         // active low, bit 7 = decimal point
    output wire [7:0]  HEX1,
    output wire [7:0]  HEX2,
    output wire [7:0]  HEX3,
    output wire [7:0]  HEX4,
    output wire [7:0]  HEX5,

    output wire [3:0]  VGA_R,
    output wire [3:0]  VGA_G,
    output wire [3:0]  VGA_B,
    output wire        VGA_HS,
    output wire        VGA_VS,

    inout  wire [35:0] GPIO
);

    // -----------------------------------------------------------------------
    // Clock and reset
    // -----------------------------------------------------------------------
    wire clk14, pll_locked;

    pll u_pll (
        .areset (1'b0),
        .inclk0 (MAX10_CLK1_50),
        .c0     (clk14),
        .locked (pll_locked)
    );

    reg [7:0] rst_ctr = 8'd0;
    wire      rst = ~rst_ctr[7];

    always @(posedge clk14) begin
        if (!pll_locked || !KEY[0]) rst_ctr <= 8'd0;
        else if (!rst_ctr[7])       rst_ctr <= rst_ctr + 8'd1;
    end

    // -----------------------------------------------------------------------
    // Joystick on GPIO[4:0]; beeper on GPIO[35]; everything else undriven
    // -----------------------------------------------------------------------
    wire speaker;

    assign GPIO[34:5] = 30'bz;         // includes PS/2 pins: never driven
    assign GPIO[4:0]  = 5'bz;              // inputs (weak pull-ups in the qsf)
    assign GPIO[35]   = speaker;

    wire [4:0] joy_state;
    wire [7:0] kempston;

    // PS/2 keyboard -- receive-only; both lines are inputs with weak pull-ups
    wire [7:0]  ps2_code;
    wire        ps2_valid;
    wire [39:0] ps2_matrix;

    ps2_rx u_ps2_rx (
        .clk (clk14), .rst (rst),
        .ps2_clk (GPIO[26]), .ps2_data (GPIO[27]),
        .code (ps2_code), .valid (ps2_valid), .frame_err ()
    );

    ps2_keyboard u_ps2_map (
        .clk (clk14), .rst (rst),
        .code (ps2_code), .valid (ps2_valid),
        .key_matrix (ps2_matrix)
    );

    joystick #(.DEBOUNCE_CYCLES(14000)) u_joystick (
        .clk      (clk14),
        .rst      (rst),
        .pin_n    (GPIO[4:0]),
        .state    (joy_state),
        .kempston (kempston)
    );

    // -----------------------------------------------------------------------
    // The machine
    // -----------------------------------------------------------------------
    wire [3:0] r, g, b;
    wire       hs, vs;
    wire [2:0] border;
    wire       mic;

    speccy48 #(.ROM_FILE("rom48.hex")) u_speccy (
        .clk        (clk14),
        .rst        (rst),
        .key_matrix (ps2_matrix),
        .joy_state  (joy_state),
        .ear_in     (1'b1),
        .speaker    (speaker),
        .mic        (mic),
        .border     (border),
        .vga_r      (r),
        .vga_g      (g),
        .vga_b      (b),
        .vga_hsync  (hs),
        .vga_vsync  (vs),
        .vga_blank  ()
    );

    assign VGA_R = r;
    assign VGA_G = g;
    assign VGA_B = b;

    // Most monitors want negative sync at this rate; SW[9] flips it live.
    assign VGA_HS = SW[9] ? hs : ~hs;
    assign VGA_VS = SW[9] ? vs : ~vs;

    // -----------------------------------------------------------------------
    // Signs of life
    // -----------------------------------------------------------------------
    reg [23:0] heartbeat = 24'd0;
    always @(posedge clk14) heartbeat <= heartbeat + 24'd1;

    assign LEDR[4:0] = joy_state;
    assign LEDR[7:5] = border;
    assign LEDR[8]   = pll_locked;
    assign LEDR[9]   = heartbeat[23];

    assign HEX0 = seg7(kempston[3:0]);
    assign HEX1 = seg7(kempston[7:4]);
    assign HEX2 = 8'hFF;
    assign HEX3 = 8'hFF;
    assign HEX4 = 8'hFF;
    assign HEX5 = 8'hFF;

    function [7:0] seg7(input [3:0] v);
        case (v)
            4'h0: seg7 = ~8'h3F;  4'h1: seg7 = ~8'h06;
            4'h2: seg7 = ~8'h5B;  4'h3: seg7 = ~8'h4F;
            4'h4: seg7 = ~8'h66;  4'h5: seg7 = ~8'h6D;
            4'h6: seg7 = ~8'h7D;  4'h7: seg7 = ~8'h07;
            4'h8: seg7 = ~8'h7F;  4'h9: seg7 = ~8'h6F;
            4'hA: seg7 = ~8'h77;  4'hB: seg7 = ~8'h7C;
            4'hC: seg7 = ~8'h39;  4'hD: seg7 = ~8'h5E;
            4'hE: seg7 = ~8'h79;  default: seg7 = ~8'h71;
        endcase
    endfunction

endmodule

`default_nettype wire
