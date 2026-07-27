// ---------------------------------------------------------------------------
// Bus testbench: the harness pretends to be the Z80.
//
// This verifies everything the CPU plugs into -- memory map, ULA ports,
// keyboard matrix, Kempston, interrupt timing -- before a CPU core is chosen.
// If the machine later misbehaves with a real Z80 in the socket, the fault is
// in the CPU, not here.
// ---------------------------------------------------------------------------

#include <verilated.h>
#include "Vspeccy_tb_top.h"

#include <cstdio>
#include <cstdint>
#include <cstring>

static Vspeccy_tb_top* top;
static int failures = 0;

static void check(const char* what, uint32_t got, uint32_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL: %-30s got 0x%02X, expected 0x%02X\n", what, got, want);
        failures++;
    } else {
        printf("  ok  %-30s 0x%02X\n", what, got);
    }
}

static void tick(int n = 1) {
    for (int i = 0; i < n; i++) {
        top->clk = 0; top->eval();
        top->clk = 1; top->eval();
    }
}

// The design is synchronous at 14 MHz; a Z80 bus cycle is several of those.
// Holding the control lines for a few clocks and sampling at the end is a
// faithful enough stand-in for real M-cycle timing.
static void idle() {
    top->mreq_n = 1; top->iorq_n = 1; top->rd_n = 1; top->wr_n = 1; top->m1_n = 1;
    tick(2);
}

static uint8_t mem_read(uint16_t a) {
    top->cpu_a = a; top->mreq_n = 0; top->rd_n = 0;
    tick(4);
    uint8_t d = top->cpu_di;
    idle();
    return d;
}

static void mem_write(uint16_t a, uint8_t d) {
    top->cpu_a = a; top->cpu_do = d; top->mreq_n = 0; top->wr_n = 0;
    tick(4);
    idle();
}

static uint8_t io_read(uint16_t a) {
    top->cpu_a = a; top->iorq_n = 0; top->rd_n = 0;
    tick(4);
    uint8_t d = top->cpu_di;
    idle();
    return d;
}

static void io_write(uint16_t a, uint8_t d) {
    top->cpu_a = a; top->cpu_do = d; top->iorq_n = 0; top->wr_n = 0;
    tick(4);
    idle();
}

// Must match the generator in the Makefile.
static uint8_t test_rom_byte(int i) { return (uint8_t)((i * 7 + 3) & 0xFF); }

// key_matrix bit for half-row `row` (0 = A8 .. 7 = A15), key `bit` (0..4)
static uint64_t key(int row, int bit) { return 1ULL << (row * 5 + bit); }

int main(int argc, char** argv) {
    VerilatedContext ctx;
    ctx.commandArgs(argc, argv);
    top = new Vspeccy_tb_top(&ctx);

    top->rst = 1; top->key_matrix = 0; top->joy_state = 0; top->ear_in = 1;
    top->cpu_a = 0; top->cpu_do = 0;
    idle();
    tick(16);
    top->rst = 0;
    tick(16);

    // ---- memory map ------------------------------------------------------
    printf("memory map\n");
    check("ROM 0x0000",        mem_read(0x0000), test_rom_byte(0x0000));
    check("ROM 0x1234",        mem_read(0x1234), test_rom_byte(0x1234));
    check("ROM 0x3FFF",        mem_read(0x3FFF), test_rom_byte(0x3FFF));

    // ROM must ignore writes -- this is the one that silently breaks BASIC.
    mem_write(0x1234, 0x5A);
    check("ROM ignores writes", mem_read(0x1234), test_rom_byte(0x1234));

    mem_write(0x4000, 0xA5);
    mem_write(0x7FFF, 0x3C);
    check("screen RAM 0x4000",  mem_read(0x4000), 0xA5);
    check("screen RAM 0x7FFF",  mem_read(0x7FFF), 0x3C);

    mem_write(0x8000, 0x11);
    mem_write(0xFFFF, 0x22);
    mem_write(0xC000, 0x33);
    check("upper RAM 0x8000",   mem_read(0x8000), 0x11);
    check("upper RAM 0xFFFF",   mem_read(0xFFFF), 0x22);
    check("upper RAM 0xC000",   mem_read(0xC000), 0x33);

    // Banks must not alias onto each other.
    check("0x4000 unchanged",   mem_read(0x4000), 0xA5);
    check("ROM still intact",   mem_read(0x0000), test_rom_byte(0x0000));

    // ---- ULA port write --------------------------------------------------
    printf("port 0xFE write\n");
    io_write(0x00FE, 0x02);
    check("border = red",       top->border, 2);
    check("speaker low",        top->speaker, 0);

    io_write(0x00FE, 0x1F);     // border 7, MIC and speaker set
    check("border = white",     top->border, 7);
    check("speaker high",       top->speaker, 1);
    check("mic high",           top->mic, 1);

    // Any even port reaches the ULA, not just 0xFE.
    io_write(0x00F4, 0x03);
    check("even port decodes",  top->border, 3);

    // ---- keyboard --------------------------------------------------------
    printf("keyboard\n");
    // Idle: nothing pressed, all rows read 1s. Bits 7 and 5 read high, bit 6
    // follows EAR, which is high here.
    check("no keys, A8 low",    io_read(0xFEFE), 0xFF);

    // CAPS SHIFT is row A8 (address bit 8 low), bit 0.
    top->key_matrix = key(0, 0);
    check("CAPS on A8",         io_read(0xFEFE), 0xFE);
    check("not seen on A9",     io_read(0xFDFE), 0xFF);

    // "V" is row A8 bit 4.
    top->key_matrix = key(0, 4);
    check("V on A8",            io_read(0xFEFE), 0xEF);

    // SPACE is row A15 bit 0 -- the far end of the matrix.
    top->key_matrix = key(7, 0);
    check("SPACE on A15",       io_read(0x7FFE), 0xFE);
    check("not seen on A8",     io_read(0xFEFE), 0xFF);

    // Selecting every half-row at once is how the ROM scans for any key.
    check("SPACE, all rows",    io_read(0x00FE), 0xFE);

    // Two keys in different rows, both rows selected: results OR together.
    top->key_matrix = key(0, 0) | key(7, 1);
    check("CAPS+SYM, all rows", io_read(0x00FE), 0xFC);

    // EAR appears on bit 6.
    top->key_matrix = 0;
    top->ear_in = 0;
    check("EAR low",            io_read(0xFEFE), 0xBF);
    top->ear_in = 1;

    // ---- Kempston --------------------------------------------------------
    printf("Kempston\n");
    check("joystick idle",      io_read(0x001F), 0x00);

    top->joy_state = 0x08;      // right   {fire,right,left,down,up}
    check("right",              io_read(0x001F), 0x01);
    top->joy_state = 0x01;      // up
    check("up",                 io_read(0x001F), 0x08);
    top->joy_state = 0x10;      // fire
    check("fire",               io_read(0x001F), 0x10);
    top->joy_state = 0x1F;
    check("all directions",     io_read(0x001F), 0x1F);
    top->joy_state = 0;

    // An unclaimed port must float high, not answer.
    check("unused port 0x00FD", io_read(0x00FD), 0xFF);

    // ---- interrupt -------------------------------------------------------
    printf("interrupt\n");
    idle();

    // Measure one full period, and how long INT stays asserted.
    long gap = 0, len = 0, seen = 0;
    bool prev = top->int_n;
    for (long i = 0; i < 8L * 139776 && seen < 2; i++) {
        tick();
        const bool now = top->int_n;
        if (prev && !now) {             // falling edge: interrupt asserted
            if (seen == 0) gap = 0;
            seen++;
        } else if (!prev && now) {      // rising edge: released
            // length measured in 14 MHz clocks
        }
        if (seen == 1) { gap++; if (!now) len++; }
        prev = now;
    }

    // One interrupt per frame. A frame is 448*312 = 139776 pixels at 7 MHz,
    // and the clock is 14 MHz, so that is 279552 clocks -- 19.968 ms, 50.08 Hz.
    const long FRAME_CLKS = 2L * 448 * 312;
    printf("  interrupt period %ld clks (expect %ld, = %.2f Hz), length %ld clks\n",
           gap, FRAME_CLKS, 14.0e6 / (double)gap, len);
    if (gap != FRAME_CLKS) {
        fprintf(stderr, "FAIL: interrupt period %ld, expected %ld\n", gap, FRAME_CLKS);
        failures++;
    }
    // 32 T-states at 3.5 MHz = 128 clocks at 14 MHz.
    if (len < 120 || len > 136) {
        fprintf(stderr, "FAIL: interrupt length %ld clks, expected ~128\n", len);
        failures++;
    } else {
        printf("  ok  interrupt length within tolerance of 32 T-states\n");
    }

    // An interrupt acknowledge (IORQ with M1 low) must not be answered by the
    // ULA -- if it is, the vector is corrupted and IM 2 breaks.
    top->cpu_a = 0x00FE; top->iorq_n = 0; top->rd_n = 0; top->m1_n = 0;
    tick(4);
    check("IORQ+M1 not ULA read", top->cpu_di, 0xFF);
    idle();

    top->final();
    delete top;
    printf(failures ? "FAILED (%d)\n" : "ok\n", failures);
    return failures ? 1 : 0;
}
