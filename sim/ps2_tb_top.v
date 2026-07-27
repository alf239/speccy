// PS/2 chain under test: receiver + scancode mapper, as wired on the board.

`default_nettype none

module ps2_tb_top (
    input  wire        clk,
    input  wire        rst,
    input  wire        ps2_clk,
    input  wire        ps2_data,
    output wire [39:0] key_matrix,
    output wire        rx_valid,
    output wire        rx_err
);

    wire [7:0] code;

    ps2_rx #(.WATCHDOG(5600)) u_rx (
        .clk (clk), .rst (rst),
        .ps2_clk (ps2_clk), .ps2_data (ps2_data),
        .code (code), .valid (rx_valid), .frame_err (rx_err)
    );

    ps2_keyboard u_map (
        .clk (clk), .rst (rst),
        .code (code), .valid (rx_valid),
        .key_matrix (key_matrix)
    );

endmodule

`default_nettype wire
