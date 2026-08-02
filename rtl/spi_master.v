// ---------------------------------------------------------------------------
// spi_master -- mode 0, 8-bit, MSB first; the divMMC's SPI port
//
// A write to the divMMC data port (0xEB) latches the byte and starts an
// exchange; the byte clocked in during the same exchange is what a later
// read returns. esxDOS receives by transmitting 0xFF, per SD convention.
//
// Mode 0: SCK idles low, MOSI is valid before the rising edge, MISO is
// sampled on the rising edge, shifting happens on the falling edge.
//
// Two speeds, selected at any time:
//   speed=0   ~350 kHz  (clk/40)  -- SD cards demand <=400 kHz during init
//   speed=1    7 MHz    (clk/2)   -- data transfers
//
// /CS is NOT handled here -- on a divMMC it is just a register bit on port
// 0xE7, so it lives with the port logic.
// ---------------------------------------------------------------------------

`default_nettype none

module spi_master (
    input  wire       clk,           // 14 MHz
    input  wire       rst,

    input  wire       speed,         // 0 = init (~350 kHz), 1 = fast (7 MHz)
    input  wire       start,         // pulse: begin an 8-bit exchange
    input  wire [7:0] tx,
    output reg  [7:0] rx,
    output reg        busy,

    output reg        sck,
    output wire       mosi,
    input  wire       miso
);

    // Half-period in clk cycles, minus one for the counter compare.
    wire [4:0] half = speed ? 5'd0 : 5'd19;

    reg [7:0] shreg;
    reg [3:0] bits;                  // bits remaining
    reg [4:0] div;
    reg       miso_s;                // sampled on the rising edge

    assign mosi = shreg[7];

    always @(posedge clk) begin
        if (rst) begin
            busy  <= 1'b0;
            sck   <= 1'b0;
            shreg <= 8'hFF;
            bits  <= 4'd0;
            div   <= 5'd0;
            rx    <= 8'hFF;
        end else if (!busy) begin
            sck <= 1'b0;
            if (start) begin
                shreg <= tx;
                bits  <= 4'd8;
                div   <= 5'd0;
                busy  <= 1'b1;
            end
        end else if (div != half) begin
            div <= div + 5'd1;
        end else begin
            div <= 5'd0;
            if (!sck) begin
                // Rising edge: sample MISO.
                sck    <= 1'b1;
                miso_s <= miso;
            end else begin
                // Falling edge: shift; byte complete when this was bit 1.
                sck   <= 1'b0;
                shreg <= {shreg[6:0], miso_s};
                bits  <= bits - 4'd1;
                if (bits == 4'd1) begin
                    rx   <= {shreg[6:0], miso_s};
                    busy <= 1'b0;
                end
            end
        end
    end

endmodule

`default_nettype wire
