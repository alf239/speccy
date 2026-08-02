// Test wrapper: the full machine with the smoke-test ROM, plus debug taps
// into memory so the testbench can verify what the CPU wrote. Verilator
// flattens the hierarchy, so constant-index reads into another module's
// memory array are free.

`default_nettype none

module speccy48_tb_top (
    input  wire        clk,
    input  wire        rst,

    input  wire [39:0] key_matrix,
    input  wire [4:0]  joy_state,
    input  wire        ear_in,
    output wire        speaker,
    output wire        mic,
    output wire [2:0]  border,
    output wire        int_n_obs,   // observed interrupt line

    // Debug taps -- addresses match sim/make_cpu_rom.py
    output wire [7:0]  dbg_bitmap,  // 0x4000 -> vram[0x0000]
    output wire [7:0]  dbg_attr,    // 0x5800 -> vram[0x1800]
    output wire [7:0]  dbg_ramhi,   // 0x8000 -> ramhi[0x0000]
    output wire [7:0]  dbg_intctr,  // 0x9000 -> ramhi[0x1000]

    // Bus visibility for debugging
    output wire [15:0] dbg_a,
    output wire [7:0]  dbg_di,
    output wire        dbg_m1_n,
    output wire        dbg_mreq_n,
    output wire        dbg_rd_n,
    output wire        dbg_ce_cpu
);

    speccy48 #(.ROM_FILE("sim/cpu_rom.hex")) u_ss (
        .clk          (clk),
        .rst          (rst),
        .divmmc_en    (1'b0),
        .nmi_button   (1'b0),
        .sd_cs (), .sd_sck (), .sd_mosi (), .sd_miso (1'b1), .dbg_sd (),
        .arm_snapshot (1'b0),
        .key_matrix (key_matrix),
        .joy_state  (joy_state),
        .ear_in     (ear_in),
        .speaker    (speaker),
        .mic        (mic),
        .border     (border),
        .vga_r      (),
        .vga_g      (),
        .vga_b      (),
        .vga_hsync  (),
        .vga_vsync  (),
        .vga_blank  ()
    );

    assign int_n_obs  = u_ss.u_machine.int_n;

    assign dbg_a      = u_ss.cpu_a;
    assign dbg_di     = u_ss.cpu_di;
    assign dbg_m1_n   = u_ss.m1_n;
    assign dbg_mreq_n = u_ss.mreq_n;
    assign dbg_rd_n   = u_ss.rd_n;
    assign dbg_ce_cpu = u_ss.ce_cpu;

    assign dbg_bitmap = u_ss.u_machine.u_vram.mem[14'h0000];
    assign dbg_attr   = u_ss.u_machine.u_vram.mem[14'h1800];
    assign dbg_ramhi  = u_ss.u_machine.u_ramhi.mem[15'h0000];
    assign dbg_intctr = u_ss.u_machine.u_ramhi.mem[15'h1000];

endmodule

`default_nettype wire
