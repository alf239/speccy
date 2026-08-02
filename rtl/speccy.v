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
    parameter SNAP_FILE  = "",     // 64K combined snapshot image for divRAM
    parameter DIVMMC_ROM = "",     // 8K esxDOS image; empty = no divMMC
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

    // divMMC runtime enable (board switch). With this low -- or with no
    // DIVMMC_ROM built in -- the machine is exactly the plain 48K.
    input  wire        divmmc_en,

    // High while the boot copier is refilling RAM from the snapshot shadow.
    // The CPU must be held in reset while this is set -- block RAM init only
    // happens at CONFIGURATION, so without the copy a second armed reset
    // would resume the snapshot's registers over whatever RAM holds now.
    output wire        boot_busy,

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
    // Held low to stretch an I/O cycle while the SPI exchange completes --
    // this is what makes the 350 kHz init clock safe against back-to-back
    // OUT/IN sequences in esxDOS. Feed to the CPU's wait_n.
    output wire        cpu_wait_n,

    // ---- peripherals ----------------------------------------------------
    input  wire [39:0] key_matrix,   // active high, 8 half-rows of 5
    input  wire [4:0]  joy_state,    // active high {fire,right,left,down,up}
    input  wire        ear_in,       // tape input
    output reg         speaker,
    output reg         mic,
    output wire [2:0]  border,

    // ---- SD card (SPI) --------------------------------------------------
    output wire        sd_cs,
    output wire        sd_sck,
    output wire        sd_mosi,
    input  wire        sd_miso,

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

    // Boot copier signals (driven by the snapshot logic below; idle when the
    // machine has no snapshot support). Source data comes from the divRAM,
    // whose configuration-time contents are the snapshot image.
    wire        copying;
    wire [15:0] caddr;       // 0..49151 over the copy
    wire        cwrite;      // write strobe, data valid on divram_q

    assign boot_busy = copying;

    // The 0x4000 bank needs two ports: CPU on one, video on the other. The
    // boot copier borrows the CPU port while the CPU is held in reset.
    vram #(.ADDR_W(14), .INIT_FILE(VRAM_FILE)) u_vram (
        .clk    (clk),
        .a_addr (copying ? caddr[13:0] : cpu_a[13:0]),
        .a_din  (copying ? divram_q : cpu_do),
        .a_we   (copying ? (cwrite && !caddr[15] && !caddr[14])
                         : (mem_wr && (cpu_a[15:14] == 2'b01))),
        .a_dout (vram_q),
        .b_addr (video_addr),
        .b_dout (video_data)
    );

    // Modulo-32K arithmetic makes the top bit irrelevant.
    wire [14:0] caddr_hi = caddr[14:0] - 15'd16384;

    ram #(.ADDR_W(15), .INIT_FILE(RAM_FILE)) u_ramhi (
        .clk  (clk),
        .addr (copying ? caddr_hi : cpu_a[14:0]),
        .din  (copying ? divram_q : cpu_do),
        .we   (copying ? (cwrite && (caddr[15] || caddr[14]))
                       : (mem_wr && cpu_a[15])),
        .dout (ramhi_q)
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

            // Two clocks per byte (read divRAM shadow, then write main): 48K
            // in ~7 ms at 14 MHz, done during what looks like a long reset.
            reg        copy_r;
            reg        cphase;              // 0: shadow read, 1: main write
            reg [15:0] ctr;

            always @(posedge clk) begin
                if (rst) begin
                    copy_r <= arm_snapshot;
                    cphase <= 1'b0;
                    ctr    <= 16'd0;
                end else if (copy_r) begin
                    cphase <= ~cphase;
                    if (cphase) begin
                        if (ctr == 16'd49151) copy_r <= 1'b0;
                        ctr <= ctr + 16'd1;
                    end
                end
            end

            assign copying = copy_r;
            assign caddr   = ctr;
            assign cwrite  = copy_r && cphase;
        end else begin : g_no_overlay
            assign stub_q  = 8'h00;
            assign copying = 1'b0;
            assign caddr   = 16'd0;
            assign cwrite  = 1'b0;
        end
    endgenerate

    // -----------------------------------------------------------------------
    // divMMC: esxDOS ROM + 32K banked RAM behind an automapper.
    //
    // The automapper takes effect AFTER the M1 cycle that hits an entry
    // point -- the trapped instruction itself still executes from the
    // Spectrum ROM, and the following fetch lands in esxDOS. Exit points
    // (0x1FF8-0x1FFF) unmap the same way, so the RET there executes from
    // esxDOS ROM and returns into Spectrum ROM. This delay is the designed
    // behaviour of divIDE/divMMC; esxDOS is assembled around it.
    //
    // Port 0xE3: bit7 CONMEM (manual map), bit6 MAPRAM (sticky until reset:
    // divRAM bank 3 replaces the ROM, write-protected), bits1:0 bank at
    // 0x2000-0x3FFF. Port 0xE7 bit0: SD /CS. Port 0xEB: SPI data exchange.
    // -----------------------------------------------------------------------
    localparam HAS_DIVMMC = (DIVMMC_ROM != "");
    localparam HAS_AUX    = (STUB_FILE != "") || (DIVMMC_ROM != "");

    wire        div_on = divmmc_en && HAS_DIVMMC;
    wire [7:0]  esx_q, divram_q;
    reg         conmem, mapram, automap;
    reg  [2:0]  divbank;           // 8 banks x 8K = 64K (esxDOS uses 5+)
    reg         sd_cs_r;
    reg         spi_start;
    reg  [7:0]  spi_tx;
    wire [7:0]  spi_rx;
    wire        spi_busy;
    wire        eb_r_lat;          // read latched + background exchange running
    wire [7:0]  eb_rd_hold;        // the byte that read will return

    // M1 fetch tracking: decisions latch during the fetch, apply at its end.
    wire m1_fetch = mem_rd && !m1_n;
    reg  m1_fetch_d;
    reg  [15:0] fetch_a;
    always @(posedge clk) begin
        m1_fetch_d <= m1_fetch;
        if (m1_fetch) fetch_a <= cpu_a;
    end
    wire fetch_end = m1_fetch_d && !m1_fetch;
    // Two trap classes, per divIDE/divMMC spec: entry/exit points switch
    // AFTER their M1 cycle (the trapped instruction runs from the old map),
    // but 0x3D00-0x3DFF maps INSTANTLY -- the fetch itself must read divRAM,
    // which is how both TR-DOS emulation and esxDOS's ROM-call trampoline
    // (a RET planted at 0x3DFD) work. The sticky automap still latches at
    // fetch end via entry_hit.
    wire instant_3d = div_on && m1_fetch && (cpu_a[15:8] == 8'h3D);
    wire divmap = (div_on && (automap || conmem)) || instant_3d;


    wire entry_hit = (fetch_a == 16'h0000) || (fetch_a == 16'h0008) ||
                     (fetch_a == 16'h0038) || (fetch_a == 16'h0066) ||
                     (fetch_a == 16'h04C6) || (fetch_a == 16'h0562) ||
                     (fetch_a[15:8] == 8'h3D);
    wire exit_hit  = (fetch_a[15:3] == 13'h03FF);        // 0x1FF8-0x1FFF

    // divRAM: one 64K array serving two masters. Banked 8K pages for the
    // divMMC window (bank 3 stands in for the ROM in MAPRAM mode), and the
    // snapshot shadow for the boot copier, which owns the port while the
    // CPU is held in reset. Initialised from the snapshot image at
    // configuration -- so esxDOS use overwrites the shadow, degrading
    // snapshot replay to once-per-programming until reprogrammed.
    wire        div_low   = divmap && (cpu_a[15:13] == 3'b000);  // 0x0000-1FFF
    wire        div_win   = divmap && (cpu_a[15:13] == 3'b001);  // 0x2000-3FFF
    wire [15:0] divram_a  = copying ? caddr :
                            div_low ? {3'd3, cpu_a[12:0]}
                                    : {divbank, cpu_a[12:0]};
    wire        bank3_wp  = mapram && (div_low || (div_win && divbank == 3'd3));
    wire        divram_we = !copying && mem_wr && div_win && !bank3_wp;

    // I/O decode (full low byte; A0=1 so the ULA never collides)
    wire io_e3 = div_on && (cpu_a[7:0] == 8'hE3);
    wire io_e7 = div_on && (cpu_a[7:0] == 8'hE7);
    wire io_eb = div_on && (cpu_a[7:0] == 8'hEB);

    generate
        if (HAS_AUX) begin : g_divram
            ram #(.ADDR_W(16), .INIT_FILE(SNAP_FILE)) u_divram (
                .clk (clk), .addr (divram_a), .din (cpu_do),
                .we (divram_we), .dout (divram_q)
            );
        end else begin : g_no_divram
            /* verilator lint_off UNUSEDSIGNAL */
            wire _aux_unused = &{divram_a, divram_we};
            /* verilator lint_on UNUSEDSIGNAL */
            assign divram_q = 8'hFF;
        end
    endgenerate

    generate
        if (HAS_DIVMMC) begin : g_divmmc
            ram #(.ADDR_W(13), .INIT_FILE(DIVMMC_ROM)) u_esxrom (
                .clk (clk), .addr (cpu_a[12:0]), .din (8'd0),
                .we (1'b0), .dout (esx_q)
            );

            // SD init demands <=400 kHz; after 1024 exchanges (well past any
            // init conversation) the clock steps up to 7 MHz. The WAIT
            // stretch below makes the slow phase invisible to software.
            reg [10:0] spi_cnt;
            wire spi_fast = spi_cnt[10];

            reg  eb_started;
            reg  eb_r_started;
            reg  [7:0] eb_rd_val;
            wire io_any_eb = (io_rd || io_wr_raw) && io_eb;

            always @(posedge clk) begin
                if (rst) begin
                    conmem  <= 1'b0;
                    mapram  <= 1'b0;
                    automap <= 1'b0;
                    divbank <= 3'd0;
                    sd_cs_r <= 1'b1;
                    spi_cnt <= 11'd0;
                    spi_start  <= 1'b0;
                    spi_tx     <= 8'hFF;
                    eb_started <= 1'b0;
                    eb_r_started <= 1'b0;
                    eb_rd_val  <= 8'hFF;
                end else begin
                    spi_start <= 1'b0;

                    if (fetch_end) begin
                        if (entry_hit)     automap <= 1'b1;
                        else if (exit_hit) automap <= 1'b0;
                    end

                    if (io_wr && io_e3) begin
                        conmem  <= cpu_do[7];
                        mapram  <= mapram | cpu_do[6];   // sticky
                        divbank <= cpu_do[2:0];
                    end
                    // CS must not change mid-exchange: at the slow init
                    // clock a preamble byte is still shifting when esxDOS
                    // reaches for CS, and a mid-byte select would desync the
                    // card's framing by the orphaned edges. The WAIT stretch
                    // below holds the OUT until the wire is quiet.
                    if (io_wr_raw && io_e7 && !spi_busy)
                        sd_cs_r <= cpu_do[0];

                    // Start an exchange once per (possibly stretched) OUT.
                    if (io_wr_raw && io_eb && !spi_busy && !eb_started) begin
                        spi_tx     <= cpu_do;
                        spi_start  <= 1'b1;
                        eb_started <= 1'b1;
                        if (!spi_fast) spi_cnt <= spi_cnt + 11'd1;
                    end
                    // A READ returns the latched byte from the previous
                    // exchange AND clocks a new 0xFF one -- the divMMC idiom
                    // that lets one IN instruction stream one byte.
                    if (io_rd && io_eb && !spi_busy && !eb_r_started) begin
                        eb_rd_val    <= spi_rx;
                        spi_tx       <= 8'hFF;
                        spi_start    <= 1'b1;
                        eb_r_started <= 1'b1;
                        if (!spi_fast) spi_cnt <= spi_cnt + 11'd1;
                    end
                    if (!io_any_eb) begin
                        eb_started   <= 1'b0;
                        eb_r_started <= 1'b0;
                    end
                end
            end

            assign eb_r_lat   = eb_r_started;
            assign eb_rd_hold = eb_rd_val;

            spi_master u_spi (
                .clk (clk), .rst (rst),
                .speed (spi_fast),
                .start (spi_start), .tx (spi_tx),
                .rx (spi_rx), .busy (spi_busy),
                .sck (sd_sck), .mosi (sd_mosi), .miso (sd_miso)
            );
        end else begin : g_no_divmmc
            // Without a divMMC the trap/port plumbing has no consumers.
            /* verilator lint_off UNUSEDSIGNAL */
            wire _div_unused = &{sd_miso, spi_start, spi_tx, fetch_end,
                                 entry_hit, exit_hit, io_e3, io_e7};
            /* verilator lint_on UNUSEDSIGNAL */
            assign esx_q    = 8'hFF;
            assign spi_rx   = 8'hFF;
            assign spi_busy = 1'b0;
            assign eb_r_lat   = 1'b0;
            assign eb_rd_hold = 8'hFF;
            assign sd_sck   = 1'b0;
            assign sd_mosi  = 1'b1;
            always @(posedge clk) begin
                conmem <= 1'b0; mapram <= 1'b0; automap <= 1'b0;
                divbank <= 3'd0; sd_cs_r <= 1'b1;
                spi_start <= 1'b0; spi_tx <= 8'hFF;
            end
        end
    endgenerate

    assign sd_cs = sd_cs_r;

    // Stretch 0xEB accesses only until the PREVIOUS exchange completes: a
    // read latches that byte and kicks off the next transfer in the
    // background; a write queues behind the one in flight.
    wire eb_rd_wait = io_rd && io_eb && spi_busy && !eb_r_lat;
    wire eb_wr_wait = io_wr_raw && io_eb && spi_busy;
    wire e7_wait    = io_wr_raw && io_e7 && spi_busy;
    assign cpu_wait_n = !(eb_rd_wait || eb_wr_wait || e7_wait);

    // The RAMs register their outputs, so the bank select has to be delayed to
    // match or the mux picks the wrong one.
    reg [1:0] rd_bank;
    reg       rd_stub, rd_divrom, rd_divram;
    always @(posedge clk) begin
        rd_bank   <= cpu_a[15:14];
        rd_stub   <= overlay_armed && (cpu_a[15:8] == 8'h00);
        rd_divrom <= div_low && !mapram;
        rd_divram <= (div_low && mapram) || div_win;
    end

    wire [7:0] mem_data = rd_stub             ? stub_q   :
                          rd_divrom           ? esx_q    :
                          rd_divram           ? divram_q :
                          (rd_bank == 2'b00)  ? rom_q    :
                          (rd_bank == 2'b01)  ? vram_q   : ramhi_q;

    // -----------------------------------------------------------------------
    // Ports
    //
    // IORQ with M1 low is an interrupt acknowledge, not an I/O cycle -- if that
    // is not excluded, the ULA answers the acknowledge and corrupts the vector.
    // -----------------------------------------------------------------------
    wire io_rd     = !iorq_n && !rd_n && m1_n;
    wire io_wr_raw = !iorq_n && !wr_n && m1_n;
    // Registered ports must latch once per cycle even when WAIT stretches it.
    reg  io_wr_d;
    always @(posedge clk) io_wr_d <= io_wr_raw;
    wire io_wr = io_wr_raw && !io_wr_d;

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
                    io_rd  ? (io_eb        ? (eb_r_lat ? eb_rd_hold : spi_rx) :
                              ula_sel      ? ula_data :
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
