// ---------------------------------------------------------------------------
// joystick -- Atari-standard DE-9 stick to a Kempston port byte
//
// The stick is five passive switches to ground, so this is the one peripheral
// on the whole machine that needs no level shifting: enable weak pull-ups on
// the FPGA pins and wire the connector straight in. Nothing ever drives 5V
// because nothing in the stick drives anything at all.
//
//   DE-9 pin   function      wire to
//   --------   --------      -------
//      1       up            pin_n[0]
//      2       down          pin_n[1]
//      3       left          pin_n[2]
//      4       right         pin_n[3]
//      6       fire          pin_n[4]
//      8       ground        GND
//
// (Pin 9 is a second fire button on some sticks. Kempston has no bit for it.)
//
// "Kempston compatible" is a property of the interface, not the stick: any
// Atari-standard DE-9 joystick works, because the interface is what decides
// which bits the switches land on.
//
// Kempston reads as port 0x1F, active high, 000FUDLR:
//
//   bit 4 fire   bit 3 up   bit 2 down   bit 1 left   bit 0 right
//
// Note the inversion -- switches are active low, the port is active high.
// ---------------------------------------------------------------------------

`default_nettype none

module joystick #(
    // Mechanical switches bounce for a few milliseconds. Require the input to
    // hold still this long before believing it.
    parameter DEBOUNCE_CYCLES = 14000    // ~1 ms at 14 MHz
)(
    input  wire       clk,
    input  wire       rst,
    input  wire [4:0] pin_n,       // asynchronous, active low: {fire,right,left,down,up}

    output reg  [4:0] state,       // debounced, active high, same bit order
    output wire [7:0] kempston     // port 0x1F read value
);

    // Two-stage synchroniser: these pins are asynchronous to everything.
    reg [4:0] sync0, sync1;
    always @(posedge clk) begin
        sync0 <= ~pin_n;           // invert here, so everything downstream is active high
        sync1 <= sync0;
    end

    // Debounce the whole vector together: any change restarts the timer.
    reg [$clog2(DEBOUNCE_CYCLES+1)-1:0] timer;

    always @(posedge clk) begin
        if (rst) begin
            state <= 5'd0;
            timer <= 0;
        end else if (sync1 == state) begin
            timer <= 0;
        end else if (timer == DEBOUNCE_CYCLES[$clog2(DEBOUNCE_CYCLES+1)-1:0]) begin
            state <= sync1;
            timer <= 0;
        end else begin
            timer <= timer + 1'b1;
        end
    end

    //                     fire      up        down      left      right
    assign kempston = {3'b000, state[4], state[0], state[1], state[2], state[3]};

endmodule

`default_nettype wire
