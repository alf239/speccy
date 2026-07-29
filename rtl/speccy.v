// ---------------------------------------------------------------------------
// speccy -- the 48K machine, minus the CPU
//
// Everything a Z80 plugs into: memory map, ULA ports, keyboard, Kempston,
// interrupt generation, and the video chain. The CPU bus is brought out as
// ports so the testbench can drive it directly, which means all of this can be
// verified before choosing a CPU core -- and later, if the machine misbehaves
// with a real Z80 in the socket, we already know the fault is in the CPU.
//
// Memory map (48K, no paging):
//   0x0000-0x3FFF  ROM, 16K, writes ignored
//   0x4000-0x7FFF  RAM, 16K, shared with video (dual-port, no contention here)
//   0x8000-0xFFFF  RAM, 32K
//
// Ports:
//   0xFE  any even port (A0 = 0)
//         write  bits 2:0 border, bit 3 MIC, bit 4 speaker
//         read   bits 4:0 keyboard half-rows selected by A8..A15, bit 6 EAR
//   0x1F  Kempston joystick, decoded on A5 = 0 (the classic decode)
//
// A port with A0 = 0 *and* A5 = 0 satisfies both decodes; the ULA wins here.
// Real hardware had the same collision and nobody minded.
//
// Timing is Pentagon-style: no memory contention. Video and CPU each have their
// own RAM port and never collide, so the CPU runs at a flat 3.5 MHz.
// ---------------------------------------------------------------------------

`default_nettype none

module speccy #(
    parameter ROM_FILE   = "",
    parameter VRAM_FILE  = "",
    parameter RAM_FILE   = "",     // upper 32K init (snapshot loading)
    parameter STUB_FILE  = "",     // 256-byte snapshot boot overlay
    // Interrupt: 32 T-states long on a 48K, asserted once per frame. The line
    // it starts on decides where "raster line 0" sits for timing-sensitive
    // code; the start of vertical blanking is a reasonable default and this is
    // a parameter so it can be moved.
    parameter INT_LENGTH = 32,
    parameter INT_LINE   = 248
)(
    input  wire        clk,          // 14 MHz
    input  wire        rst,

    // Sampled during reset: arm the snapshot overlay, which serves the stub
    // at 0x0000-0x00FF until the first opcode fetch at or above 0x0100 (the
    // stub's final JP into the snapshot's PC). Tie low for a normal boot.
    input  wire        arm_snapshot,

    output wire        ce_cpu,       // 3.5 MHz
    output wire        ce_pix,       // 7 MHz

    // ---- CPU bus --------------------------------------------------------
    input  wire [15:0] cpu_a,
    input  wire [7:0]  cpu_do,       // data from the CPU
    output wire [7:0]  cpu_di,       // data to the CPU
    input  wire        mreq_n,
    input  wire        iorq_n,
    input  wire        rd_n,
    input  wire        wr_n,
    input  wire        m1_n,
    output wire        int_n,

    // ---- peripherals ----------------------------------------------------
    input  wire [39:0] key_matrix,   // active high, 8 half-rows of 5
    input  wire [4:0]  joy_state,    // active high {fire,right,left,down,up}
    input  wire        ear_in,       // tape input
    output reg         speaker,
    output reg         mic,
    output wire [2:0]  border,

    // ---- video ----------------------------------------------------------
    output wire [3:0]  vga_r,
    output wire [3:0]  vga_g,
    output wire [3:0]  vga_b,
    output wire        vga_hsync,
    output wire        vga_vsync,
    output wire        vga_blank
);

    // -----------------------------------------------------------------------
    // Clock enables: 14 MHz / 2 = 7 MHz pixel, / 4 = 3.5 MHz CPU
    // -----------------------------------------------------------------------
    reg [1:0] phase;
    always @(posedge clk) phase <= rst ? 2'd0 : phase + 2'd1;

    assign ce_pix = phase[0];
    assign ce_cpu = (phase == 2'b11);

    // -----------------------------------------------------------------------
    // Memory
    // -----------------------------------------------------------------------
    wire mem_rd = !mreq_n && !rd_n;
    wire mem_wr = !mreq_n && !wr_n;

    wire [7:0] rom_q, vram_q, ramhi_q;
    wire [13:0] video_addr;
    wire [7:0]  video_data;

    ram #(.ADDR_W(14), .INIT_FILE(ROM_FILE)) u_rom (
        .clk (clk), .addr (cpu_a[13:0]), .din (8'd0), .we (1'b0), .dout (rom_q)
    );

    // The 0x4000 bank needs two ports: CPU on one, video on the other.
    vram #(.ADDR_W(14), .INIT_FILE(VRAM_FILE)) u_vram (
        .clk    (clk),
        .a_addr (cpu_a[13:0]),
        .a_din  (cpu_do),
        .a_we   (mem_wr && (cpu_a[15:14] == 2'b01)),
        .a_dout (vram_q),
        .b_addr (video_addr),
        .b_dout (video_data)
    );

    ram #(.ADDR_W(15), .INIT_FILE(RAM_FILE)) u_ramhi (
        .clk (clk), .addr (cpu_a[14:0]), .din (cpu_do),
        .we (mem_wr && cpu_a[15]), .dout (ramhi_q)
    );

    // -----------------------------------------------------------------------
    // Snapshot boot overlay: a 256-byte ROM shadowing 0x0000-0x00FF while
    // armed. Armed state is loaded during reset and cleared forever by the
    // first opcode fetch outside the stub -- which is the stub's own JP into
    // the game. From then on the real ROM is back.
    // -----------------------------------------------------------------------
    localparam HAS_STUB = (STUB_FILE != "");

    wire [7:0] stub_q;
    reg        overlay_armed;

    always @(posedge clk) begin
        if (rst)
            overlay_armed <= HAS_STUB ? arm_snapshot : 1'b0;
        else if (mem_rd && !m1_n && (cpu_a >= 16'h0100))
            overlay_armed <= 1'b0;
    end

    generate
        if (HAS_STUB) begin : g_overlay
            ram #(.ADDR_W(8), .INIT_FILE(STUB_FILE)) u_stub (
                .clk (clk), .addr (cpu_a[7:0]), .din (8'd0),
                .we (1'b0), .dout (stub_q)
            );
        end else begin : g_no_overlay
            assign stub_q = 8'h00;
        end
    endgenerate

    // The RAMs register their outputs, so the bank select has to be delayed to
    // match or the mux picks the wrong one.
    reg [1:0] rd_bank;
    reg       rd_stub;
    always @(posedge clk) begin
        rd_bank <= cpu_a[15:14];
        rd_stub <= overlay_armed && (cpu_a[15:8] == 8'h00);
    end

    wire [7:0] mem_data = rd_stub             ? stub_q  :
                          (rd_bank == 2'b00)  ? rom_q   :
                          (rd_bank == 2'b01)  ? vram_q  : ramhi_q;

    // -----------------------------------------------------------------------
    // Ports
    //
    // IORQ with M1 low is an interrupt acknowledge, not an I/O cycle -- if that
    // is not excluded, the ULA answers the acknowledge and corrupts the vector.
    // -----------------------------------------------------------------------
    wire io_rd = !iorq_n && !rd_n && m1_n;
    wire io_wr = !iorq_n && !wr_n && m1_n;

    wire ula_sel      = !cpu_a[0];
    wire kempston_sel = !cpu_a[5];

    reg [2:0] border_r;
    assign border = border_r;

    always @(posedge clk) begin
        if (rst) begin
            border_r <= 3'd0;
            speaker  <= 1'b0;
            mic      <= 1'b0;
        end else if (io_wr && ula_sel) begin
            border_r <= cpu_do[2:0];
            mic      <= cpu_do[3];
            speaker  <= cpu_do[4];
        end
    end

    wire [4:0] kb_rows;
    keyboard u_keyboard (
        .row_sel_n  (cpu_a[15:8]),
        .key_matrix (key_matrix),
        .rows       (kb_rows)
    );

    //                    bit 7   bit 6    bit 5   bits 4:0
    wire [7:0] ula_data = {1'b1, ear_in, 1'b1, kb_rows};

    //                         fire          up            down          left          right
    wire [7:0] kempston = {3'b000, joy_state[4], joy_state[0], joy_state[1], joy_state[2], joy_state[3]};

    assign cpu_di = mem_rd ? mem_data :
                    io_rd  ? (ula_sel      ? ula_data :
                              kempston_sel ? kempston : 8'hFF)
                           : 8'hFF;

    // -----------------------------------------------------------------------
    // Interrupt -- one pulse per frame, INT_LENGTH T-states long
    // -----------------------------------------------------------------------
    wire [8:0] px_h, px_v;

    wire int_start = ce_pix && (px_v == INT_LINE[8:0]) && (px_h == 9'd0);

    reg [7:0] int_ctr;
    always @(posedge clk) begin
        if (rst)                             int_ctr <= 8'd0;
        else if (int_start)                  int_ctr <= INT_LENGTH[7:0];
        else if (ce_cpu && int_ctr != 8'd0)  int_ctr <= int_ctr - 8'd1;
    end

    assign int_n = (int_ctr == 8'd0);

    // -----------------------------------------------------------------------
    // Video
    // -----------------------------------------------------------------------
    wire [3:0] px_idx;
    wire       px_vsync, px_vblank;

    video u_video (
        .clk           (clk),
        .rst           (rst),
        .ce_pix        (ce_pix),
        .border_colour (border_r),
        .vram_addr     (video_addr),
        .vram_data     (video_data),
        .px_idx        (px_idx),
        .px_hsync      (),
        .px_vsync      (px_vsync),
        .px_blank      (),
        .px_vblank     (px_vblank),
        .px_h          (px_h),
        .px_v          (px_v),
        .frame_end     ()
    );

    wire [3:0] out_idx;

    scandoubler u_scandoubler (
        .clk       (clk),
        .rst       (rst),
        .ce_pix    (ce_pix),
        .in_h      (px_h),
        .in_v      (px_v),
        .in_idx    (px_idx),
        .in_vsync  (px_vsync),
        .in_vblank (px_vblank),
        .in_v_last (9'd311),
        .out_idx   (out_idx),
        .out_h     (),
        .out_v     (),
        .out_hsync (vga_hsync),
        .out_vsync (vga_vsync),
        .out_blank (vga_blank)
    );

    palette u_palette (
        .idx (out_idx), .blank (vga_blank),
        .r (vga_r), .g (vga_g), .b (vga_b)
    );

endmodule

`default_nettype wire
