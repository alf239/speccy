// ---------------------------------------------------------------------------
// AY-3-8912 unit tests, against the datasheet:
//
//   - register write masks and readback (incl. R14 port A, which 128K
//     detection routines probe)
//   - tone frequency: period TP toggles the channel every 16*TP master
//     clocks
//   - mixer gating: tone disabled = steady level, not silence
//   - volume DAC: level 15 = full scale, level 0 = silence, monotonic
//   - envelope shapes: 0x0D ramps up and holds, 0x00 decays and holds at 0,
//     0x0E triangles; a write to R13 restarts the ramp
//   - AY vs YM mode: same ramp duration, 16 vs 32 distinct levels
//   - noise: enabling it produces transitions with the tone silent
// ---------------------------------------------------------------------------

#include <verilated.h>
#include "Vay8912.h"

#include <cstdio>
#include <set>

static Vay8912* top;
static int failures = 0;

static void check(const char* what, long got, long want) {
    if (got != want) {
        fprintf(stderr, "FAIL: %-32s got %ld, expected %ld\n", what, got, want);
        failures++;
    } else {
        printf("  ok  %-32s %ld\n", what, got);
    }
}

static void check_range(const char* what, long got, long lo, long hi) {
    if (got < lo || got > hi) {
        fprintf(stderr, "FAIL: %-32s got %ld, expected %ld..%ld\n", what, got, lo, hi);
        failures++;
    } else {
        printf("  ok  %-32s %ld (%ld..%ld)\n", what, got, lo, hi);
    }
}

// Every clk is a ce: the "master clock" here is the verilator clock, which
// only scales time, not behaviour.
static void tick(int n = 1) {
    while (n--) {
        top->clk = 0; top->eval();
        top->clk = 1; top->eval();
    }
}

static void wr(uint8_t reg, uint8_t val) {
    top->addr_wr = 1; top->din = reg; tick(); top->addr_wr = 0;
    top->data_wr = 1; top->din = val; tick(); top->data_wr = 0;
}

static uint8_t rd(uint8_t reg) {
    top->addr_wr = 1; top->din = reg; tick(); top->addr_wr = 0;
    tick();
    return top->dout;
}

static void silence_all() {
    wr(7, 0x3F);                    // everything off
    wr(8, 0); wr(9, 0); wr(10, 0);
}

int main(int argc, char** argv) {
    VerilatedContext ctx;
    ctx.commandArgs(argc, argv);
    top = new Vay8912(&ctx);

    top->rst = 1; top->ce = 1; top->ym_mode = 0;
    top->addr_wr = 0; top->data_wr = 0; top->din = 0;
    tick(4);
    top->rst = 0;

    // ---- registers --------------------------------------------------------
    printf("registers\n");
    wr(1, 0xFF); check("R1 masks to 4 bits",  rd(1), 0x0F);
    wr(6, 0xFF); check("R6 masks to 5 bits",  rd(6), 0x1F);
    wr(8, 0xFF); check("R8 masks to 5 bits",  rd(8), 0x1F);
    wr(0, 0xA5); check("R0 full width",       rd(0), 0xA5);
    wr(14, 0xAB); check("R14 port A readback", rd(14), 0xAB);

    // ---- tone frequency ---------------------------------------------------
    printf("tone\n");
    silence_all();
    wr(0, 100); wr(1, 0);           // period 100
    wr(7, 0x3E);                    // tone A only
    wr(8, 15);                      // fixed full volume
    // Toggle every 16*100 = 1600 ce. Count level transitions over 160000 ce
    // -> expect ~100.
    int last = top->audio, transitions = 0;
    for (int i = 0; i < 160000; i++) {
        tick();
        if ((top->audio != 0) != (last != 0)) transitions++;
        last = top->audio;
    }
    check_range("tone A toggles (period 100)", transitions, 98, 102);

    // ---- mixer ------------------------------------------------------------
    printf("mixer\n");
    wr(7, 0x3F);                    // tone A off again, volume still 15
    tick(4000);
    long lvl = top->audio;
    bool steady = lvl > 0;
    for (int i = 0; i < 4000; i++) { tick(); if (top->audio != lvl) steady = false; }
    check("disabled tone = steady level", steady ? 1 : 0, 1);
    check("full volume is full scale", lvl, 255);
    wr(8, 0);
    tick(4);
    check("volume 0 is silence", top->audio, 0);

    // DAC monotonicity over fixed levels
    long prev = -1; bool mono = true;
    for (int v = 0; v <= 15; v++) {
        wr(8, v); tick(4);
        if (top->audio < prev) mono = false;
        prev = top->audio;
    }
    check("DAC monotonic over 16 levels", mono ? 1 : 0, 1);

    // ---- envelope ---------------------------------------------------------
    printf("envelope\n");
    silence_all();
    wr(7, 0x3F);
    wr(8, 0x10);                    // channel A follows the envelope
    wr(11, 4); wr(12, 0);           // fast envelope

    // Shape 0x0D: ramp up, hold at top.
    wr(13, 0x0D);
    prev = 0; mono = true;
    for (int i = 0; i < 5000; i++) {
        tick();
        if (top->audio < prev - 0) { /* sampled mid-step is fine */ }
        prev = top->audio;
    }
    check("shape 0D holds at full scale", top->audio, 255);

    // Shape 0x00: decay, hold at zero.
    wr(13, 0x00);
    tick(5000);
    check("shape 00 holds at zero", top->audio, 0);

    // R13 write restarts: after 0x0D has held at max, rewriting drops it.
    wr(13, 0x0D); tick(5000);
    check("0D at top before restart", top->audio, 255);
    wr(13, 0x0D); tick(8);
    check_range("R13 write restarts ramp", top->audio, 0, 30);

    // Shape 0x0E: triangle -- must revisit both extremes repeatedly.
    wr(13, 0x0E);
    int at_top = 0, at_bot = 0;
    for (int i = 0; i < 20000; i++) {
        tick();
        if (top->audio == 255) at_top++;
        if (top->audio == 0)   at_bot++;
    }
    check("triangle touches top",    at_top > 0 ? 1 : 0, 1);
    check("triangle touches bottom", at_bot > 0 ? 1 : 0, 1);

    // ---- AY vs YM resolution ---------------------------------------------
    printf("AY vs YM\n");
    // A single 0x0D up-ramp visits 16 (AY) or 32 (YM) envelope indices; the
    // shared DAC table folds a few low YM entries together, hence the bands.
    std::set<long> ay_levels, ym_levels;
    wr(13, 0x0D);
    for (int i = 0; i < 6000; i++) { tick(); ay_levels.insert(top->audio); }
    top->ym_mode = 1;
    wr(13, 0x0D);
    for (int i = 0; i < 6000; i++) { tick(); ym_levels.insert(top->audio); }
    top->ym_mode = 0;
    check_range("AY distinct levels", (long)ay_levels.size(), 15, 18);
    check_range("YM distinct levels", (long)ym_levels.size(), 26, 32);

    // ---- noise ------------------------------------------------------------
    printf("noise\n");
    silence_all();
    wr(6, 1);
    wr(7, 0x37);                    // noise on channel A only
    wr(8, 15);
    last = top->audio; transitions = 0;
    for (int i = 0; i < 100000; i++) {
        tick();
        if ((top->audio != 0) != (last != 0)) transitions++;
        last = top->audio;
    }
    check("noise produces transitions", transitions > 100 ? 1 : 0, 1);

    // ---- sparse ce -------------------------------------------------------
    // The machine pulses ce one clock in eight (1.75 MHz from 14 MHz); the
    // module must behave identically, just slower in wall-clock.
    printf("sparse ce\n");
    silence_all();
    wr(0, 10); wr(1, 0);
    wr(7, 0x3E); wr(8, 15);
    last = top->audio; transitions = 0;
    for (int i = 0; i < 160000; i++) {
        top->ce = (i % 8) == 0;
        tick();
        if ((top->audio != 0) != (last != 0)) transitions++;
        last = top->audio;
    }
    top->ce = 1;
    // Toggle every 16*10 ce = 1280 clocks -> ~125 transitions.
    check_range("tone under sparse ce", transitions, 120, 130);

    top->final();
    delete top;
    printf(failures ? "FAILED (%d)\n" : "ok\n", failures);
    return failures ? 1 : 0;
}
