// ---------------------------------------------------------------------------
// CPU smoke test: TV80 executing real code against the real machine.
//
// The ROM (sim/make_cpu_rom.py) sets the border, writes to screen and upper
// RAM, then counts IM 1 interrupts into 0x9000. If the counter tracks the
// number of INT pulses the machine generated, the whole chain works: fetch,
// execute, memory map, OUT, INT timing, acknowledge, vectoring, EI/RET.
// ---------------------------------------------------------------------------

#include <verilated.h>
#include "Vspeccy48_tb_top.h"
#include "sdram_model.h"

#include <cstdio>
#include <cstdint>

static const long FRAME_CLKS = 2L * 448 * 312;   // 14 MHz clocks per frame

static Vspeccy48_tb_top* top;
static int failures = 0;

static void check(const char* what, long got, long want) {
    if (got != want) {
        fprintf(stderr, "FAIL: %-28s got 0x%02lX, expected 0x%02lX\n", what, got, want);
        failures++;
    } else {
        printf("  ok  %-28s 0x%02lX\n", what, got);
    }
}

int main(int argc, char** argv) {
    VerilatedContext ctx;
    ctx.commandArgs(argc, argv);
    top = new Vspeccy48_tb_top(&ctx);

    bool m128 = false;
    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--128")) m128 = true;
    SdramModel sdram_chip;

    printf("=== %s mode ===\n", m128 ? "128K" : "48K");
    top->rst = 1; top->key_matrix = 0; top->joy_state = 0; top->ear_in = 1;
    top->en_128 = m128 ? 1 : 0;
    top->dram_dq_in = 0xFFFF;
    for (int i = 0; i < 16; i++) { top->clk = 0; top->eval(); top->clk = 1; top->eval(); }
    top->rst = 0;

    const int FRAMES = 6;
    long ints_seen = 0, audio_changes = 0;
    bool prev_int = top->int_n_obs;
    int  prev_audio = top->audio;

    for (long i = 0; i < FRAMES * FRAME_CLKS; i++) {
        top->clk = 0; top->eval();
        top->clk = 1; top->eval();
        uint16_t dq = 0xFFFF;
        sdram_chip.step(top->dram_cs_n, top->dram_ras_n, top->dram_cas_n,
                        top->dram_we_n, top->dram_addr, top->dram_ba,
                        top->dram_ldqm, top->dram_udqm,
                        top->dram_dq_out, top->dram_dq_oe, dq);
        top->dram_dq_in = dq;
        const bool now = top->int_n_obs;
        if (prev_int && !now) ints_seen++;
        prev_int = now;
        if (top->audio != prev_audio) audio_changes++;
        prev_audio = top->audio;
    }

    printf("ran %d frames, saw %ld interrupts\n", FRAMES, ints_seen);

    check("border red (OUT 0xFE)", top->border,     0x02);
    check("bitmap byte at 0x4000", top->dbg_bitmap, 0xAA);
    check("attr byte at 0x5800",   top->dbg_attr,   0x55);
    if (m128) {
        // 0x8000 is SDRAM bank 2 now -- the M9K tap must NOT see the marker,
        // and the paging probe covers the SDRAM readback path instead.
        check("ramhi untouched in 128K", top->dbg_ramhi, 0x00);
        check("paging: bank 0 byte",     top->dbg_pg0,   0xA0);
        check("paging: bank 1 byte",     top->dbg_pg1,   0xA1);
        check("no SDRAM protocol errors", (long)sdram_chip.errors, 0);
    } else {
        check("upper RAM at 0x8000",   top->dbg_ramhi,  0x5A);
        // 7FFD inert: both probe writes landed on one ramhi cell.
        check("paging probe inert (0)", top->dbg_pg0,   0xA1);
        check("paging probe inert (1)", top->dbg_pg1,   0xA1);
    }
    check("CALL into RAM executes",    top->dbg_exec,  0x5A);
    check("speaker still low",     top->speaker,    0x00);

    // The ROM programs the AY over OUT (C),A: tone A, period 50, volume 15.
    // 2187.5 Hz over ~0.12 s of run = several hundred audio transitions.
    printf("audio bus changed %ld times\n", audio_changes);
    check("CPU-driven AY tone plays", audio_changes > 100 ? 1 : 0, 1);

    // The counter must track the INT pulses. EI happens within the first few
    // hundred T-states and the first INT is ~248 lines in, so every pulse
    // should be serviced; allow one in flight at the moment we stopped.
    // In 128K mode 0x9000 is SDRAM (bank 2) -- read the counter out of the
    // model's memory; the ISR incrementing it every frame is a nice 50 Hz
    // read-modify-write soak of the SDRAM path.
    const long ctr = m128 ? (long)(sdram_chip.mem[0x9000 >> 1] & 0xFF)
                          : (long)top->dbg_intctr;
    printf("interrupt counter 0x%02lX\n", ctr);
    if (ints_seen < FRAMES - 1) {
        fprintf(stderr, "FAIL: expected ~%d INT pulses, saw %ld\n", FRAMES, ints_seen);
        failures++;
    }
    if (ctr != ints_seen && ctr != ints_seen - 1) {
        fprintf(stderr, "FAIL: counter 0x%02lX does not track %ld INT pulses\n", ctr, ints_seen);
        failures++;
    } else {
        printf("  ok  IM 1 handler ran once per frame\n");
    }

    top->final();
    delete top;
    printf(failures ? "FAILED (%d)\n" : "ok\n", failures);
    return failures ? 1 : 0;
}
