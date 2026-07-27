// ---------------------------------------------------------------------------
// ps2_rx -- PS/2 receiver, receive-only
//
// The keyboard drives both lines (open collector); we only listen. A PS/2
// frame is 11 bits, sent LSB-first, clocked by the KEYBOARD's clock at
// 10-16.7 kHz, data valid on the falling edge:
//
//   start(0)  d0 d1 d2 d3 d4 d5 d6 d7  parity(odd)  stop(1)
//
// Receive-only is enough for keys: keyboards power up scanning in set 2 and
// stream make/break codes with no host initialisation. (Host-to-device --
// setting the lock LEDs, typematic rate -- would need open-drain drive and a
// request-to-send state machine. Not needed for phase 1.)
//
// A watchdog resets the bit counter if the keyboard's clock stops mid-frame
// for ~400 us, so a glitch cannot leave the receiver misaligned forever.
// ---------------------------------------------------------------------------

`default_nettype none

module ps2_rx #(
    parameter WATCHDOG = 5600          // ~400 us at 14 MHz
)(
    input  wire       clk,             // 14 MHz
    input  wire       rst,

    input  wire       ps2_clk,         // asynchronous, from the keyboard
    input  wire       ps2_data,

    output reg  [7:0] code,            // received byte
    output reg        valid,           // one-clk pulse: `code` is good
    output reg        frame_err        // one-clk pulse: start/stop/parity bad
);

    // Two-stage synchronisers; these lines are asynchronous and slow.
    reg [1:0] clk_sync, dat_sync;
    always @(posedge clk) begin
        clk_sync <= {clk_sync[0], ps2_clk};
        dat_sync <= {dat_sync[0], ps2_data};
    end

    wire sclk = clk_sync[1];
    wire sdat = dat_sync[1];

    reg  sclk_d;
    always @(posedge clk) sclk_d <= sclk;
    wire fall = sclk_d && !sclk;

    reg [3:0] nbits;
    reg [9:0] shreg;                   // newest bit shifts in from the top
    reg [$clog2(WATCHDOG+1)-1:0] idle;

    // The full 11-bit frame as it stands after this falling edge:
    // {stop, parity, d7..d0, start}
    wire [10:0] frame = {sdat, shreg};

    always @(posedge clk) begin
        valid     <= 1'b0;
        frame_err <= 1'b0;

        if (rst) begin
            nbits <= 4'd0;
            idle  <= 0;
        end else begin
            // Watchdog: a stalled frame is abandoned, not completed later.
            if (fall) idle <= 0;
            else if (idle != WATCHDOG[$clog2(WATCHDOG+1)-1:0]) idle <= idle + 1'b1;
            if (idle == WATCHDOG[$clog2(WATCHDOG+1)-1:0]) nbits <= 4'd0;

            if (fall) begin
                shreg <= frame[10:1];
                if (nbits == 4'd10) begin
                    nbits <= 4'd0;
                    //        start        stop     odd parity over d0..d7+parity
                    if (!frame[0] && frame[10] && (^frame[9:1])) begin
                        code  <= frame[8:1];
                        valid <= 1'b1;
                    end else begin
                        frame_err <= 1'b1;
                    end
                end else begin
                    nbits <= nbits + 4'd1;
                end
            end
        end
    end

endmodule

`default_nettype wire
