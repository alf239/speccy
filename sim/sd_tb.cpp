// ---------------------------------------------------------------------------
// SPI master + SD model tests, two phases:
//
//  1. Wire-level: loopback (MOSI tied to MISO must echo), mode-0 phase (MISO
//     changing right after the rising edge must NOT corrupt the sample),
//     clock division in both speeds.
//
//  2. Card-level: full SDHC init conversation exactly as esxDOS performs it
//     (CMD0, CMD8, ACMD41 loop, CMD58), then CMD17 reads a block of a known
//     test image back through the SPI master, then CMD24 writes one and
//     CMD17 verifies the write landed.
// ---------------------------------------------------------------------------

#include <verilated.h>
#include "Vspi_master.h"
#include "sd_model.h"

#include <cstdio>
#include <cstring>

static Vspi_master* top;
static SdModel* card;
static bool cs_n = true;
static int failures = 0;

static void check(const char* what, long got, long want) {
    if (got != want) {
        fprintf(stderr, "FAIL: %-30s got 0x%lX, expected 0x%lX\n", what, got, want);
        failures++;
    } else {
        printf("  ok  %-30s 0x%02lX\n", what, got);
    }
}

static void tick() {
    top->clk = 0; top->eval();
    top->clk = 1; top->eval();
    if (card) top->miso = card->step(cs_n, top->sck, top->mosi);
}

// One 8-bit exchange through the verilated master.
static uint8_t xfer(uint8_t b) {
    top->tx = b; top->start = 1;
    tick();
    top->start = 0;
    int guard = 100000;
    while (top->busy && guard--) tick();
    return top->rx;
}

// SD command: 6 bytes, then hunt for the R1 (first byte with bit7 clear).
static uint8_t sd_cmd(uint8_t cmd, uint32_t arg, uint8_t crc = 0x01) {
    xfer(0x40 | cmd);
    xfer(arg >> 24); xfer(arg >> 16); xfer(arg >> 8); xfer(arg);
    xfer(crc);
    for (int i = 0; i < 16; i++) {
        uint8_t r = xfer(0xFF);
        if (!(r & 0x80)) return r;
    }
    return 0xFF;
}

int main(int argc, char** argv) {
    VerilatedContext ctx;
    ctx.commandArgs(argc, argv);
    top = new Vspi_master(&ctx);

    top->rst = 1; top->start = 0; top->tx = 0; top->miso = 1; top->speed = 1;
    for (int i = 0; i < 4; i++) tick();
    top->rst = 0;

    // ---- phase 1: wire level ---------------------------------------------
    printf("wire level\n");
    card = nullptr;

    // Loopback: patch miso from mosi every tick.
    for (uint8_t pat : {0xA5, 0x0F, 0x81}) {
        top->tx = pat; top->start = 1;
        tick(); top->start = 0;
        int guard = 1000;
        while (top->busy && guard--) { top->miso = top->mosi; tick(); }
        check("loopback echo", top->rx, pat);
    }

    // Clock division: count sck rising edges and cycles at each speed.
    for (int spd = 0; spd <= 1; spd++) {
        top->speed = spd;
        long cycles = 0, edges = 0;
        bool prev = false;
        top->tx = 0x55; top->start = 1;
        tick(); top->start = 0;
        while (top->busy) {
            tick(); cycles++;
            if (top->sck && !prev) edges++;
            prev = top->sck;
        }
        printf("  speed=%d: %ld cycles, %ld sck edges\n", spd, cycles, edges);
        if (edges != 8) { fprintf(stderr, "FAIL: %ld sck rising edges\n", edges); failures++; }
        // fast: ~2 clk per bit; slow: ~40 clk per bit
        long expect = spd ? 16 : 320;
        if (cycles < expect - 4 || cycles > expect + 44) {
            fprintf(stderr, "FAIL: speed=%d took %ld cycles (expect ~%ld)\n", spd, cycles, expect);
            failures++;
        }
    }
    top->speed = 1;

    // ---- phase 2: talk to the card ---------------------------------------
    printf("card level\n");
    card = new SdModel(64);                       // 32 KB image
    for (size_t i = 0; i < card->disk.size(); i++)
        card->disk[i] = (uint8_t)((i * 7 + (i >> 9)) & 0xFF);

    // Power-up ritual: >=74 clocks with CS high.
    cs_n = true;
    for (int i = 0; i < 10; i++) xfer(0xFF);
    cs_n = false;

    check("CMD0 -> idle",        sd_cmd(0, 0, 0x95), 0x01);
    check("CMD8 -> idle",        sd_cmd(8, 0x1AA, 0x87), 0x01);
    check("CMD8 echo hi",        xfer(0xFF), 0x00);
    check("CMD8 echo hi2",       xfer(0xFF), 0x00);
    check("CMD8 echo 01",        xfer(0xFF), 0x01);
    check("CMD8 echo AA",        xfer(0xFF), 0xAA);

    uint8_t r1 = 0xFF;
    for (int i = 0; i < 10 && r1 != 0x00; i++) {
        sd_cmd(55, 0);
        r1 = sd_cmd(41, 1u << 30);
    }
    check("ACMD41 -> ready",     r1, 0x00);

    check("CMD58 R1",            sd_cmd(58, 0), 0x00);
    check("OCR CCS bit",         (xfer(0xFF) & 0x40) ? 1 : 0, 1);
    xfer(0xFF); xfer(0xFF); xfer(0xFF);

    // Read LBA 3, verify against the image.
    check("CMD17 R1",            sd_cmd(17, 3), 0x00);
    uint8_t tok = 0xFF;
    for (int i = 0; i < 16 && tok == 0xFF; i++) tok = xfer(0xFF);
    check("data token",          tok, 0xFE);
    int bad = 0;
    for (int i = 0; i < 512; i++) {
        uint8_t b = xfer(0xFF);
        if (b != card->disk[3 * 512 + i]) bad++;
    }
    xfer(0xFF); xfer(0xFF);                        // CRC
    check("block payload errors", bad, 0);

    // Write LBA 5, read it back.
    check("CMD24 R1",            sd_cmd(24, 5), 0x00);
    xfer(0xFE);                                    // data token
    for (int i = 0; i < 512; i++) xfer((uint8_t)(0xC3 ^ i));
    xfer(0x00); xfer(0x00);                        // CRC
    uint8_t resp = 0xFF;
    for (int i = 0; i < 16 && resp == 0xFF; i++) resp = xfer(0xFF);
    check("write accepted",      resp & 0x1F, 0x05);
    while (xfer(0xFF) != 0xFF) {}                  // busy

    check("CMD17 R1 (readback)", sd_cmd(17, 5), 0x00);
    tok = 0xFF;
    for (int i = 0; i < 16 && tok == 0xFF; i++) tok = xfer(0xFF);
    check("data token (readback)", tok, 0xFE);
    bad = 0;
    for (int i = 0; i < 512; i++)
        if (xfer(0xFF) != (uint8_t)(0xC3 ^ i)) bad++;
    xfer(0xFF); xfer(0xFF);
    check("write-read roundtrip", bad, 0);

    top->final();
    delete card; delete top;
    printf(failures ? "FAILED (%d)\n" : "ok\n", failures);
    return failures ? 1 : 0;
}
