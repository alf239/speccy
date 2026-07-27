// ---------------------------------------------------------------------------
// vram -- true dual-port RAM, 16 KB
//
// This is the Spectrum's video bank (0x4000-0x7FFF), addressed here from 0.
// Port A is the CPU side (read/write); port B is the video side (read only).
//
// The split matters architecturally: video needs two bytes per character cell
// (bitmap + attribute) and gets them by time-multiplexing its single port, so
// that the CPU keeps a port of its own. That is what lets the video generator
// read screen memory in the same cycle as the CPU, exactly like real hardware,
// with no arbitration anywhere.
//
// Registered read on both ports -- infers M9K on MAX 10.
// ---------------------------------------------------------------------------

`default_nettype none

module vram #(
    parameter ADDR_W = 14,                  // 16 KB
    parameter INIT_FILE = ""
)(
    input  wire                clk,

    // Port A -- CPU side
    input  wire [ADDR_W-1:0]   a_addr,
    input  wire [7:0]          a_din,
    input  wire                a_we,
    output reg  [7:0]          a_dout,

    // Port B -- video side
    input  wire [ADDR_W-1:0]   b_addr,
    output reg  [7:0]          b_dout
);

    // Quartus honours $readmemh for RAM initialisation, so the same file works
    // in simulation and in synthesis. If your Quartus version refuses, use a
    // .mif instead via the vendor attribute:
    //
    //   reg [7:0] mem [0:...] /* synthesis ram_init_file = "screen.mif" */;
    //
    // tools/ emits both formats from the same screen, so either path works.
    reg [7:0] mem [0:(1<<ADDR_W)-1];

    generate
        if (INIT_FILE != "") begin : g_init
            initial $readmemh(INIT_FILE, mem);
        end
    endgenerate

    always @(posedge clk) begin
        if (a_we) mem[a_addr] <= a_din;
        a_dout <= mem[a_addr];
        b_dout <= mem[b_addr];
    end

endmodule

`default_nettype wire
