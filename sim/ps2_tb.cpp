// ---------------------------------------------------------------------------
// PS/2 testbench: bit-bang keyboard frames into the receiver + mapper and
// check the resulting Spectrum matrix bits.
//
// Covers: single keys, break codes, chorded mappings (Backspace = CAPS+0),
// E0-extended arrows, the E0 12 fake shift (must be ignored), and a
// deliberately corrupted parity bit (must be dropped, and the next good
// frame must still land).
// ---------------------------------------------------------------------------

#include <verilated.h>
#include "Vps2_tb_top.h"

#include <cstdio>
#include <cstdint>
#include <vector>

static Vps2_tb_top* top;
static int failures = 0;

static const int HALF = 100;   // PS/2 half-period in 14 MHz clks (fast for sim)

static void tick(int n = 1) {
    for (int i = 0; i < n; i++) {
        top->clk = 0; top->eval();
        top->clk = 1; top->eval();
    }
}

// One PS/2 frame, keyboard-style: data set while clock high, sampled on the
// falling edge. flip_parity corrupts the frame for the error test.
static void send_byte(uint8_t code, bool flip_parity = false) {
    int ones = 0;
    for (int i = 0; i < 8; i++) ones += (code >> i) & 1;
    int parity = (ones & 1) ? 0 : 1;             // odd parity
    if (flip_parity) parity ^= 1;

    std::vector<int> bits = {0};                 // start
    for (int i = 0; i < 8; i++) bits.push_back((code >> i) & 1);
    bits.push_back(parity);
    bits.push_back(1);                           // stop

    for (int b : bits) {
        top->ps2_data = b; tick(HALF / 2);
        top->ps2_clk  = 0; tick(HALF);
        top->ps2_clk  = 1; tick(HALF / 2);
    }
    top->ps2_data = 1;
    tick(HALF);
}

static void send(std::initializer_list<uint8_t> seq) {
    for (uint8_t c : seq) send_byte(c);
}

static void check(const char* what, uint64_t got, uint64_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL: %-28s matrix 0x%010llX, expected 0x%010llX\n",
                what, (unsigned long long)got, (unsigned long long)want);
        failures++;
    } else {
        printf("  ok  %-28s 0x%010llX\n", what, (unsigned long long)got);
    }
}

static uint64_t matrix() { return top->key_matrix; }
static uint64_t bit(int n) { return 1ULL << n; }

int main(int argc, char** argv) {
    VerilatedContext ctx;
    ctx.commandArgs(argc, argv);
    top = new Vps2_tb_top(&ctx);

    top->rst = 1; top->ps2_clk = 1; top->ps2_data = 1;
    tick(8);
    top->rst = 0;
    tick(8);

    check("idle",                 matrix(), 0);

    send({0x1C});                                  // A make
    check("A pressed",            matrix(), bit(5));
    send({0xF0, 0x1C});                            // A break
    check("A released",           matrix(), 0);

    send({0x12, 0x1C});                            // Shift+A held
    check("shift+A",              matrix(), bit(0) | bit(5));
    send({0xF0, 0x1C, 0xF0, 0x12});
    check("all released",         matrix(), 0);

    send({0x66});                                  // Backspace = CAPS+0
    check("backspace chord",      matrix(), bit(0) | bit(20));
    send({0xF0, 0x66});
    check("backspace released",   matrix(), 0);

    send({0xE0, 0x75});                            // Up = CAPS+7
    check("up arrow chord",       matrix(), bit(0) | bit(23));
    send({0xE0, 0xF0, 0x75});
    check("up arrow released",    matrix(), 0);

    send({0xE0, 0x12});                            // fake shift: ignore
    check("fake shift ignored",   matrix(), 0);
    send({0xE0, 0xF0, 0x12});
    check("fake unshift ignored", matrix(), 0);

    send({0x52});                                  // ' -> SYM+P
    check("quote chord",          matrix(), bit(36) | bit(25));
    send({0xF0, 0x52});
    check("quote released",       matrix(), 0);

    // Corrupt frame must be dropped; the receiver must recover for the next.
    send_byte(0x1C, /*flip_parity=*/true);
    check("bad parity dropped",   matrix(), 0);
    send({0x15});                                  // Q
    check("recovers after error", matrix(), bit(10));
    send({0xF0, 0x15});
    check("Q released",           matrix(), 0);

    // Unknown code: no effect, no stuck prefix state.
    send({0x07});                                  // F12
    check("unknown code ignored", matrix(), 0);
    send({0x5A});
    check("Enter after unknown",  matrix(), bit(30));
    send({0xF0, 0x5A});

    top->final();
    delete top;
    printf(failures ? "FAILED (%d)\n" : "ok\n", failures);
    return failures ? 1 : 0;
}
