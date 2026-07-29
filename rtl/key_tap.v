// ---------------------------------------------------------------------------
// key_tap -- one keypress per rising edge of a slide switch
//
// Flipping the switch up delivers a single press of fixed duration, then
// releases regardless of the switch position; flip down to re-arm. This is
// the right shape for menu keys: a slide switch held up would otherwise be
// a key held down, which menus interpret as autorepeat or a stuck key.
//
// HOLD is in clk cycles. Four frames (~80 ms) comfortably exceeds the 48K
// ROM's per-frame key scan and any game's debounce.
// ---------------------------------------------------------------------------

`default_nettype none

module key_tap #(
    parameter HOLD = 22'd1_120_000     // 4 frames at 14 MHz
)(
    input  wire clk,
    input  wire rst,
    input  wire sw,                    // slide switch, asynchronous
    output wire pressed
);

    reg [2:0]  sync;
    reg [21:0] ctr;

    always @(posedge clk) begin
        sync <= {sync[1:0], sw};       // keeps tracking during reset, so a
                                       // switch already up at boot does not
                                       // fire a spurious press on release
        if (rst)
            ctr <= 22'd0;
        else if (sync[1] && !sync[2])  // rising edge, fully synchronised
            ctr <= HOLD;
        else if (ctr != 22'd0)
            ctr <= ctr - 22'd1;
    end

    assign pressed = (ctr != 22'd0);

endmodule

`default_nettype wire
