// ---------------------------------------------------------------------------
// ram -- single-port synchronous RAM, registered read
//
// With we tied low and an INIT_FILE, this is a ROM. The 48K machine uses three
// instances: 16K ROM, and the 32K upper bank. (The 16K bank at 0x4000 needs a
// second port for video, so it uses vram.v instead.)
//
// Infers M9K on MAX 10.
// ---------------------------------------------------------------------------

`default_nettype none

module ram #(
    parameter ADDR_W    = 14,
    parameter INIT_FILE = ""
)(
    input  wire                clk,
    input  wire [ADDR_W-1:0]   addr,
    input  wire [7:0]          din,
    input  wire                we,
    output reg  [7:0]          dout
);

    // ramstyle: without it, Quartus implements small arrays (the 256-byte
    // snapshot stub) as logic-cell registers, which cannot take an init file
    // on MAX 10 -- synthesis then fails with "MIF is not supported" (276013)
    // and a register-count blowout (276003). M9K is what we want anyway.
    (* ramstyle = "M9K" *) reg [7:0] mem [0:(1<<ADDR_W)-1];

    generate
        if (INIT_FILE != "") begin : g_init
            initial $readmemh(INIT_FILE, mem);
        end
    endgenerate

    always @(posedge clk) begin
        if (we) mem[addr] <= din;
        dout <= mem[addr];
    end

endmodule

`default_nettype wire
