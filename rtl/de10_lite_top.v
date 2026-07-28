// ---------------------------------------------------------------------------
// de10_lite_top -- board wrapper for stage 1b
//
// Quartus only. Verilator does not build this: it instantiates a vendor PLL.
//
// What this gives you without a CPU:
//   * a real Spectrum screen on a VGA monitor, from a .mif loaded into block RAM
//   * border colour from SW[2:0]
//   * sync polarity switchable at runtime on SW[9] -- try both if the monitor
//     refuses to lock, rather than rebuilding
//   * the joystick on LEDR[4:0], and the Kempston port byte on HEX1/HEX0
//
// ---------------------------------------------------------------------------
// Before this will build you need two things Quartus has to provide:
//
// 1. THE PLL. IP Catalog -> Library -> Basic Functions -> Clocks; PLLs and
//    Resets -> PLL -> ALTPLL. Name it `pll`. Settings:
//       inclk0 frequency          50.000 MHz
//       c0 output                 14.000 MHz   (multiply 7, divide 25 -- exact)
//       enable 'locked' output, no areset needed
//    50 x 7/25 = 14 exactly, so there is no fractional-N error to worry about.
//
// 2. THE PIN ASSIGNMENTS. Use Terasic's own DE10-Lite pin assignment file from
//    the System CD rather than typing pin numbers by hand -- getting VGA_R[0]
//    wrong is a long afternoon. Import it, then add pull-ups for the joystick:
//
//       set_instance_assignment -name WEAK_PULL_UP_RESISTOR ON -to GPIO[0]
//       set_instance_assignment -name WEAK_PULL_UP_RESISTOR ON -to GPIO[1]
//       set_instance_assignment -name WEAK_PULL_UP_RESISTOR ON -to GPIO[2]
//       set_instance_assignment -name WEAK_PULL_UP_RESISTOR ON -to GPIO[3]
//       set_instance_assignment -name WEAK_PULL_UP_RESISTOR ON -to GPIO[4]
//
//    Terasic's file assigns pins for peripherals this design does not use
//    (SDRAM, accelerometer). Quartus warns about those and carries on; that is
//    expected, not a problem.
// ---------------------------------------------------------------------------

`default_nettype none

module de10_lite_top (
    input  wire        MAX10_CLK1_50,

    input  wire [1:0]  KEY,          // active low
    input  wire [9:0]  SW,
    output wire [9:0]  LEDR,

    output wire [7:0]  HEX0,         // active low, bit 7 is the decimal point
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
    // Clocking and reset
    // -----------------------------------------------------------------------
    wire clk14;
    wire pll_locked;

    pll u_pll (
        .areset (1'b0),
        .inclk0 (MAX10_CLK1_50),
        .c0     (clk14),
        .locked (pll_locked)
    );

    // Hold reset for a while after the PLL locks, and while KEY[0] is held.
    reg [7:0] rst_ctr = 8'd0;
    wire      rst = ~rst_ctr[7];

    always @(posedge clk14) begin
        if (!pll_locked || !KEY[0]) rst_ctr <= 8'd0;
        else if (!rst_ctr[7])       rst_ctr <= rst_ctr + 8'd1;
    end

    // -----------------------------------------------------------------------
    // Joystick -- GPIO[4:0], see the header pinout in the DE10-Lite manual.
    // DE-9: 1 up, 2 down, 3 left, 4 right, 6 fire, 8 ground.
    // -----------------------------------------------------------------------
    assign GPIO = 36'bz;             // never driven: inputs only

    wire [4:0] joy_state;
    wire [7:0] kempston;

    joystick #(.DEBOUNCE_CYCLES(14000)) u_joystick (   // ~1 ms at 14 MHz
        .clk      (clk14),
        .rst      (rst),
        // odd header column: {fire,right,left,down,up} = pins 13,7,5,3,1
        // (fire skips pin 9: the Pi cobbler's ground pour grounds it)
        .pin_n    ({GPIO[10], GPIO[6], GPIO[4], GPIO[2], GPIO[0]}),
        .state    (joy_state),
        .kempston (kempston)
    );

    // -----------------------------------------------------------------------
    // Video
    // -----------------------------------------------------------------------
    wire [3:0] r, g, b;
    wire       hs, vs;

    speccy_video_top #(.INIT_FILE("screen.hex")) u_speccy (
        .clk           (clk14),
        .rst           (rst),
        .border_colour (SW[2:0]),

        // No CPU yet -- the screen arrives via the block RAM initialisation file.
        .cpu_addr      (14'd0),
        .cpu_din       (8'd0),
        .cpu_we        (1'b0),
        .cpu_dout      (),

        .vga_r         (r),
        .vga_g         (g),
        .vga_b         (b),
        .vga_hsync     (hs),
        .vga_vsync     (vs),
        .vga_blank     (),
        .out_h         (),
        .out_v         (),

        .ce_pix        (),
        .px_idx        (),
        .px_blank      (),
        .px_h          (),
        .px_v          (),
        .frame_end     ()
    );

    // The palette already blanks RGB, so the DAC needs nothing else.
    assign VGA_R = r;
    assign VGA_G = g;
    assign VGA_B = b;

    // Most monitors want negative sync; SW[9] flips it without a rebuild.
    assign VGA_HS = SW[9] ? hs : ~hs;
    assign VGA_VS = SW[9] ? vs : ~vs;

    // -----------------------------------------------------------------------
    // Signs of life
    // -----------------------------------------------------------------------
    reg [23:0] heartbeat = 24'd0;
    always @(posedge clk14) heartbeat <= heartbeat + 24'd1;

    assign LEDR[4:0] = joy_state;
    assign LEDR[7:5] = 3'b000;
    assign LEDR[8]   = pll_locked;
    assign LEDR[9]   = heartbeat[23];      // ~0.8 Hz blink

    assign HEX0 = seg7(kempston[3:0]);
    assign HEX1 = seg7(kempston[7:4]);
    assign HEX2 = 8'hFF;
    assign HEX3 = 8'hFF;
    assign HEX4 = 8'hFF;
    assign HEX5 = 8'hFF;

    // Active low, bit 7 is the decimal point (off).
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
