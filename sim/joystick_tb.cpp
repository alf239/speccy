// ---------------------------------------------------------------------------
// Joystick testbench.
//
// The bit mapping between a DE-9 connector and a Kempston port byte is exactly
// the kind of thing that is silently wrong and stays wrong until a game walks
// left when you push right. So check every direction individually.
// ---------------------------------------------------------------------------

#include <verilated.h>
#include "Vjoystick.h"

#include <cstdio>
#include <cstdint>

static const int DEBOUNCE = 64;   // must match the parameter used for the build

static Vjoystick* top;

static void tick(int n = 1) {
    for (int i = 0; i < n; i++) {
        top->clk = 0; top->eval();
        top->clk = 1; top->eval();
    }
}

// pin_n bit order: {fire, right, left, down, up}, active low
static void press(int up, int down, int left, int right, int fire) {
    const int active = (up << 0) | (down << 1) | (left << 2) | (right << 3) | (fire << 4);
    top->pin_n = (~active) & 0x1F;
}

static int failures = 0;

static void check(const char* what, int got, int want) {
    if (got != want) {
        fprintf(stderr, "FAIL: %-22s got 0x%02X, expected 0x%02X\n", what, got, want);
        failures++;
    } else {
        printf("  ok  %-22s 0x%02X\n", what, got);
    }
}

int main(int argc, char** argv) {
    VerilatedContext ctx;
    ctx.commandArgs(argc, argv);
    top = new Vjoystick(&ctx);

    top->rst = 1; press(0, 0, 0, 0, 0); tick(4);
    top->rst = 0; tick(DEBOUNCE * 3);

    check("idle", top->kempston, 0x00);

    // Kempston: bit0 right, bit1 left, bit2 down, bit3 up, bit4 fire
    struct { const char* name; int u, d, l, r, f; int expect; } cases[] = {
        { "right",             0,0,0,1,0, 0x01 },
        { "left",              0,0,1,0,0, 0x02 },
        { "down",              0,1,0,0,0, 0x04 },
        { "up",                1,0,0,0,0, 0x08 },
        { "fire",              0,0,0,0,1, 0x10 },
        { "up+right",          1,0,0,1,0, 0x09 },
        { "down+left+fire",    0,1,1,0,1, 0x16 },
        { "all",               1,1,1,1,1, 0x1F },
        { "released",          0,0,0,0,0, 0x00 },
    };

    for (auto& c : cases) {
        press(c.u, c.d, c.l, c.r, c.f);
        tick(DEBOUNCE * 3);
        check(c.name, top->kempston, c.expect);
    }

    // Bouncing must not get through: rattle the fire button for less than the
    // debounce window each time and the output should never move.
    press(0, 0, 0, 0, 0); tick(DEBOUNCE * 3);
    int glitches = 0;
    for (int i = 0; i < 40; i++) {
        press(0, 0, 0, 0, i & 1);
        tick(DEBOUNCE / 4);
        if (top->kempston != 0x00) glitches++;
    }
    check("bounce rejected", glitches, 0);

    // ...but a genuine press still lands.
    press(0, 0, 0, 0, 1); tick(DEBOUNCE * 3);
    check("press after bounce", top->kempston, 0x10);

    top->final();
    delete top;
    printf(failures ? "FAILED (%d)\n" : "ok\n", failures);
    return failures ? 1 : 0;
}
