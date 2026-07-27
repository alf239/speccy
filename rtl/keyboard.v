// ---------------------------------------------------------------------------
// keyboard -- 48K keyboard matrix read
//
// Eight half-rows, each selected by pulling one of A8..A15 low during a read of
// port 0xFE. Each returns five keys on D0..D4, active low.
//
//   A8   CAPS SHIFT  Z  X  C  V        A15  SPACE  SYM SHIFT  M  N  B
//   A9   A  S  D  F  G                 A14  ENTER  L  K  J  H
//   A10  Q  W  E  R  T                 A13  P  O  I  U  Y
//   A11  1  2  3  4  5                 A12  0  9  8  7  6
//
// Note the mirror symmetry: rows 0-3 read left-to-right from the outside of the
// keyboard inwards, rows 4-7 right-to-left. So bit 0 of row A11 is "1" but bit 0
// of row A12 is "0", not "6".
//
// More than one half-row can be selected at once -- the ROM does exactly this to
// scan for any keypress -- so selected rows are OR-ed together before inverting.
//
// key_matrix is 8 groups of 5 bits, active high, in the order above.
// ---------------------------------------------------------------------------

`default_nettype none

module keyboard (
    input  wire [7:0]  row_sel_n,   // cpu_a[15:8], active low
    input  wire [39:0] key_matrix,  // active high, 1 = pressed
    output wire [4:0]  rows         // active low, for D0..D4
);

    integer i;
    reg [4:0] pressed;

    always @* begin
        pressed = 5'b00000;
        for (i = 0; i < 8; i = i + 1)
            if (!row_sel_n[i])
                pressed = pressed | key_matrix[i*5 +: 5];
    end

    assign rows = ~pressed;

endmodule

`default_nettype wire
