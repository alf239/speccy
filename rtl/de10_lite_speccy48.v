// ---------------------------------------------------------------------------
// de10_lite_speccy48 -- the full machine on the board
//
// Quartus only (vendor PLL). Boots the ROM in rom48.hex; with the real 48K
// ROM that means BASIC to the (c) 1982 message with nothing plugged in.
//
//   SW[9]      sync polarity (flip if the monitor won't lock)
//   SW[8]      AY envelope resolution: down = AY (16 steps), up = YM (32)
//   SW[1]      snapshot mode: ON + reset = boot straight into the snapshot
//              baked in via snap_{stub,vram,ram}.hex (tools/snap2hex.py);
//              OFF + reset = normal BASIC boot
//   SW[5:2]    autokey: hold matrix keys ENTER / SPACE / 1 / 2 while up --
//              enough to drive most game menus until the keyboard arrives
//   SW[6]      tap-key J, SW[7] tap-key S: flipping UP delivers one ~80 ms
//              keypress then releases; flip down to re-arm (menu letters)
//   KEY[0]     reset (hold)
//   LEDR[4:0]  joystick     LEDR[7:5] border     LEDR[8] PLL lock
//   LEDR[9]    heartbeat    HEX1/HEX0 Kempston port byte
//              (divMMC mode: HEX1:0 last SPI rx byte, HEX3:2 exchange ctr)
//
// All peripheral signals sit on the ODD column of the header, so the whole
// machine wires along one physical row (plus one ground wire to pin 12/30):
//
//   header  1  3  5  7 13   GPIO[0,2,4,6,10]  joystick U D L R F
//   header 11                                 5V for the keyboard
//   header 31 33             GPIO[26,28]      PS/2 clock, data
//   header 37                GPIO[32]         beeper out (RC -> 3.5mm jack)
//   header 15 19 21 23       GPIO[12,16,18,20] SD: /CS, MOSI, MISO, SCK
//
//   SW[0]      divMMC enable (needs esxdos.hex built in and an SD card)
//   KEY[1]     NMI -- the esxDOS file-browser button
//
// Pins 9,14,20,25,34,39 (GPIO[8,11,17,22,29,34]) are grounded by the Pi
// T-cobbler's ground pour (they are all GND on a Pi) -- never assign them
// as signals while the cobbler is fitted. Discovered when the Kempston
// byte read 0x10 with no joystick: pin 9 is fire, and its hole says GND.
//
// PS/2 lines go through 2.2k series resistors -- the keyboard is a 5V device
// and MAX 10 is not 5V tolerant; the resistor limits clamp-diode current to
// a safe fraction of a milliamp. Power the keyboard from 5V, not 3.3V.
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
    wire [9:0] audio;

    // First-order sigma-delta DAC at 14 MHz: the pin's average tracks the
    // 10-bit beeper+AY mix, and the existing RC into the 3.5 mm jack (plus
    // the piezo's own mechanics) filters away the carrier.
    reg [10:0] sd_acc;
    always @(posedge clk14) sd_acc <= {1'b0, sd_acc[9:0]} + {1'b0, audio};
    wire audio_out = sd_acc[10];

    wire sd_cs, sd_sck, sd_mosi;

    // Inputs and unused pins tri-stated; beeper and SD outputs driven.
    // (Cobbler-grounded pins 9/14/20/25/34/39 stay tri-stated always.)
    assign GPIO[11:0]  = 12'bz;
    assign GPIO[12]    = sd_cs;                       // pin 15
    assign GPIO[15:13] = 3'bz;
    assign GPIO[16]    = sd_mosi;                     // pin 19
    assign GPIO[17]    = 1'bz;
    assign GPIO[18]    = 1'bz;                        // MISO: input, pin 21
    assign GPIO[19]    = 1'bz;
    assign GPIO[20]    = sd_sck;                      // pin 23
    assign GPIO[31:21] = 11'bz;
    assign GPIO[35:33] = 3'bz;
    assign GPIO[32]    = audio_out;

    wire [4:0] joy_state;
    wire [7:0] kempston;

    // PS/2 keyboard -- receive-only; both lines are inputs with weak pull-ups
    wire [7:0]  ps2_code;
    wire        ps2_valid;
    wire [39:0] ps2_matrix;

    wire ps2_err;

    ps2_rx u_ps2_rx (
        .clk (clk14), .rst (rst),
        .ps2_clk (GPIO[26]), .ps2_data (GPIO[28]),
        .code (ps2_code), .valid (ps2_valid), .frame_err (ps2_err)
    );

    // ---- PS/2 diagnostics on the 7-segment displays ----------------------
    // HEX3:HEX2  last scancode received (a live keyboard shows AA at
    //            power-up -- its BAT self-test result -- before any key)
    // HEX4       counts raw clock-line edges: moves = wire alive, even if
    //            frames are garbage
    // HEX5       frame-error count (parity/framing failures)
    reg [7:0] ps2_last;
    reg [3:0] ps2_errs, ps2_act;
    reg [1:0] act_sync;

    always @(posedge clk14) begin
        if (rst) begin
            ps2_last <= 8'h00;
            ps2_errs <= 4'd0;
            ps2_act  <= 4'd0;
            act_sync <= 2'b11;
        end else begin
            act_sync <= {act_sync[0], GPIO[26]};
            if (act_sync[1] != act_sync[0]) ps2_act <= ps2_act + 4'd1;
            if (ps2_valid) ps2_last <= ps2_code;
            if (ps2_err)   ps2_errs <= ps2_errs + 4'd1;
        end
    end

    ps2_keyboard u_ps2_map (
        .clk (clk14), .rst (rst),
        .code (ps2_code), .valid (ps2_valid),
        .key_matrix (ps2_matrix)
    );

    joystick #(.DEBOUNCE_CYCLES(14000)) u_joystick (
        .clk      (clk14),
        .rst      (rst),
        // odd header column: {fire,right,left,down,up} = pins 13,7,5,3,1
        // (fire skips pin 9: the Pi cobbler's ground pour grounds it)
        .pin_n    ({GPIO[10], GPIO[6], GPIO[4], GPIO[2], GPIO[0]}),
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

    // Autokey: a few menu keys on switches, OR-ed into the matrix.
    // SW[5:2] are level-held keys; SW[7:6] are edge-triggered single taps.
    wire key_j, key_s;

    key_tap u_key_j (.clk (clk14), .rst (rst), .sw (SW[6]), .pressed (key_j));
    key_tap u_key_s (.clk (clk14), .rst (rst), .sw (SW[7]), .pressed (key_s));

    wire [39:0] autokey;
    assign autokey = ({39'd0, SW[2]} << 30)    // ENTER
                   | ({39'd0, SW[3]} << 35)    // SPACE
                   | ({39'd0, SW[4]} << 15)    // 1
                   | ({39'd0, SW[5]} << 16)    // 2
                   | ({39'd0, key_j} << 33)    // J   (row A14, key 3)
                   | ({39'd0, key_s} << 6);    // S   (row A9,  key 1)

    wire        divmode = SW[0] && !SW[1];  // snapshot mode outranks divMMC
    wire [15:0] dbg_sd;

    speccy48 #(.ROM_FILE("rom48.hex"), .VRAM_FILE("snap_vram.hex"),
               .RAM_FILE("snap_ram.hex"), .STUB_FILE("snap_stub.hex"),
               .SNAP_FILE("snap_all.hex"),
               .DIVMMC_ROM("esxdos.hex")) u_speccy (
        .clk          (clk14),
        .rst          (rst),
        .arm_snapshot (SW[1]),
        .divmmc_en    (divmode),
        .nmi_button   (!KEY[1]),
        .sd_cs        (sd_cs),
        .sd_sck       (sd_sck),
        .sd_mosi      (sd_mosi),
        .sd_miso      (GPIO[18]),
        .dbg_sd       (dbg_sd),
        .key_matrix (ps2_matrix | autokey),
        .joy_state  (joy_state),
        .ear_in     (1'b1),
        .ym_mode    (SW[8]),               // up = YM 32-step envelopes
        .speaker    (speaker),
        .audio      (audio),
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

    // divMMC mode repurposes HEX3:0 as SD diagnostics:
    //   HEX1:0  last NON-FF SPI byte read from the card (sticky, so a lone
    //           answer inside a flood of 0xFF retry hunts still shows)
    //           FF = the card has NEVER answered (wiring/contact)
    //           01 = card alive but stuck in idle (init loop)
    //           00/data = init passed, look further downstream
    //   HEX3:2  SPI exchange counter -- spins while esxDOS is talking
    // HEX5:4 stay on the PS/2 diagnostics in both modes.
    assign HEX0 = seg7(divmode ? dbg_sd[3:0]   : kempston[3:0]);
    assign HEX1 = seg7(divmode ? dbg_sd[7:4]   : kempston[7:4]);
    assign HEX2 = seg7(divmode ? dbg_sd[11:8]  : ps2_last[3:0]);
    assign HEX3 = seg7(divmode ? dbg_sd[15:12] : ps2_last[7:4]);
    assign HEX4 = seg7(ps2_act);
    assign HEX5 = seg7(ps2_errs);

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
