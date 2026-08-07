// ---------------------------------------------------------------------------
// SDRAM controller tests against the behavioural model:
//
//   - init: the model's protocol checks pass (precharge-all, 8 refreshes,
//     mode register) and ready rises
//   - byte writes and reads across both DQM lanes, neighbours untouched
//   - access latency: a read completes within the Z80 budget (<= 8 clocks)
//   - a request issued during a refresh is served correctly afterwards
//   - soak: 20k random byte accesses at Z80 pace against a shadow array,
//     with the refresh watchdog confirming the 7.8 us budget held
// ---------------------------------------------------------------------------

#include <verilated.h>
#include "Vsdram.h"
#include "sdram_model.h"

#include <cstdio>
#include <cstdlib>

static Vsdram* top;
static SdramModel* ram;
static int failures = 0;

static void check(const char* what, long got, long want) {
    if (got != want) {
        fprintf(stderr, "FAIL: %-34s got %ld, expected %ld\n", what, got, want);
        failures++;
    } else {
        printf("  ok  %-34s %ld\n", what, got);
    }
}

static void tick() {
    top->clk = 0; top->eval();
    top->clk = 1; top->eval();
    uint16_t to_ctrl = 0xFFFF;
    ram->step(top->dram_cs_n, top->dram_ras_n, top->dram_cas_n, top->dram_we_n,
              top->dram_addr, top->dram_ba, top->dram_ldqm, top->dram_udqm,
              top->dq_out, top->dq_oe, to_ctrl);
    top->dq_in = to_ctrl;
}

// Returns cycles the access took.
static int wr8(uint32_t a, uint8_t v) {
    top->addr = a; top->din = v; top->we = 1; top->req = 1;
    tick();
    top->req = 0; top->we = 0;
    int n = 1, guard = 1000;
    while (top->busy && guard--) { tick(); n++; }
    return n;
}

static int rd8(uint32_t a, uint8_t& v) {
    top->addr = a; top->we = 0; top->req = 1;
    tick();
    top->req = 0;
    int n = 1, guard = 1000;
    while (!top->valid && guard--) { tick(); n++; }
    v = top->dout;
    return n;
}

int main(int argc, char** argv) {
    VerilatedContext ctx;
    ctx.commandArgs(argc, argv);
    top = new Vsdram(&ctx);
    ram = new SdramModel();

    top->rst = 1; top->req = 0; top->we = 0; top->addr = 0; top->din = 0;
    top->dq_in = 0xFFFF;
    for (int i = 0; i < 4; i++) tick();
    top->rst = 0;

    // ---- init ------------------------------------------------------------
    printf("init\n");
    int guard = 40000;
    while (!top->ready && guard--) tick();
    check("ready rises",              top->ready, 1);
    for (int i = 0; i < 8; i++) tick();          // settle into idle
    check("model saw full init",      ram->mode_loaded ? 1 : 0, 1);
    check("init refreshes",           ram->init_refreshes, 8);
    check("no protocol errors (init)", ram->errors, 0);

    // ---- bytes and lanes -------------------------------------------------
    printf("bytes\n");
    wr8(0x000100, 0xAA);                          // even: low lane
    wr8(0x000101, 0x55);                          // odd:  high lane
    uint8_t v;
    rd8(0x000100, v); check("even byte back", v, 0xAA);
    rd8(0x000101, v); check("odd byte back",  v, 0x55);
    // Neighbours in other words untouched (model init pattern is 0xFF)
    rd8(0x000102, v); check("neighbour word intact", v, 0xFF);
    // Different row entirely
    wr8(0x013400, 0x5A);
    rd8(0x013400, v); check("across rows", v, 0x5A);
    rd8(0x000100, v); check("first row still there", v, 0xAA);

    // ---- latency ---------------------------------------------------------
    printf("latency\n");
    int cyc = rd8(0x000100, v);
    printf("      (read took %d cycles)\n", cyc);
    check("read within Z80 budget", cyc <= 8 ? 1 : 0, 1);
    cyc = wr8(0x000180, 0x77);
    check("write within Z80 budget", cyc <= 8 ? 1 : 0, 1);

    // ---- request during refresh ------------------------------------------
    printf("refresh collision\n");
    // Park until a refresh is imminent, then fire a request into it.
    bool collided = false;
    for (int attempt = 0; attempt < 300 && !collided; attempt++) {
        tick();
        // when the controller just issued AUTO REFRESH (ras&we low? no --
        // {ras,cas,we}=001), catch it live:
        if (!top->dram_ras_n && !top->dram_cas_n && top->dram_we_n) {
            int n = wr8(0x000200, 0x33);
            (void)n;
            rd8(0x000200, v);
            check("write issued during refresh", v, 0x33);
            collided = true;
        }
    }
    if (!collided) { fprintf(stderr, "FAIL: never caught a refresh\n"); failures++; }

    // ---- soak ------------------------------------------------------------
    printf("soak\n");
    static uint8_t shadow[1 << 16];
    for (int i = 0; i < (1 << 16); i++) shadow[i] = 0xFF;
    // model memory starts 0xFFFF everywhere, matching the shadow
    srand(1234);
    long mismatches = 0;
    for (int i = 0; i < 20000; i++) {
        uint32_t a = (uint32_t)(rand() & 0xFFFF);
        if (rand() & 1) {
            uint8_t d = (uint8_t)rand();
            wr8(a, d);
            shadow[a] = d;
        } else {
            rd8(a, v);
            if (v != shadow[a]) mismatches++;
        }
        // Z80 pace: a few idle cycles between accesses
        for (int k = 0; k < 8; k++) tick();
    }
    check("soak mismatches",          mismatches, 0);
    check("no protocol errors (all)", ram->errors, 0);
    printf("      (worst refresh gap %ld clocks; budget 109)\n", ram->worst_gap);
    check("refresh budget held", ram->worst_gap <= 109 ? 1 : 0, 1);

    top->final();
    delete ram; delete top;
    printf(failures ? "FAILED (%d)\n" : "ok\n", failures);
    return failures ? 1 : 0;
}
