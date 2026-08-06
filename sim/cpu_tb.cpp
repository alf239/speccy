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

    top->rst = 1; top->key_matrix = 0; top->joy_state = 0; top->ear_in = 1;
    for (int i = 0; i < 16; i++) { top->clk = 0; top->eval(); top->clk = 1; top->eval(); }
    top->rst = 0;

    const int FRAMES = 6;
    long ints_seen = 0, audio_changes = 0;
    bool prev_int = top->int_n_obs;
    int  prev_audio = top->audio;

    for (long i = 0; i < FRAMES * FRAME_CLKS; i++) {
        top->clk = 0; top->eval();
        top->clk = 1; top->eval();
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
    check("upper RAM at 0x8000",   top->dbg_ramhi,  0x5A);
    check("speaker still low",     top->speaker,    0x00);

    // The ROM programs the AY over OUT (C),A: tone A, period 50, volume 15.
    // 2187.5 Hz over ~0.12 s of run = several hundred audio transitions.
    printf("audio bus changed %ld times\n", audio_changes);
    check("CPU-driven AY tone plays", audio_changes > 100 ? 1 : 0, 1);

    // The counter must track the INT pulses. EI happens within the first few
    // hundred T-states and the first INT is ~248 lines in, so every pulse
    // should be serviced; allow one in flight at the moment we stopped.
    printf("interrupt counter 0x%02X\n", top->dbg_intctr);
    if (ints_seen < FRAMES - 1) {
        fprintf(stderr, "FAIL: expected ~%d INT pulses, saw %ld\n", FRAMES, ints_seen);
        failures++;
    }
    const long ctr = top->dbg_intctr;
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
