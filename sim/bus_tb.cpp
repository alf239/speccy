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
#include "sdram_model.h"

#include <cstdio>
#include <cstdint>
#include <cstring>

static Vspeccy_tb_top* top;
static SdramModel* sdram;
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
        uint16_t dq = 0xFFFF;
        sdram->step(top->dram_cs_n, top->dram_ras_n, top->dram_cas_n,
                    top->dram_we_n, top->dram_addr, top->dram_ba,
                    top->dram_ldqm, top->dram_udqm,
                    top->dram_dq_out, top->dram_dq_oe, dq);
        top->dram_dq_in = dq;
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
    int guard = 40000;
    while (!top->cpu_wait_n && guard--) tick();   // SDRAM reads stretch (init too)
    tick(1);
    uint8_t d = top->cpu_di;
    idle();
    return d;
}

static void mem_write(uint16_t a, uint8_t d) {
    top->cpu_a = a; top->cpu_do = d; top->mreq_n = 0; top->wr_n = 0;
    tick(4);
    int guard = 40000;
    while (!top->cpu_wait_n && guard--) tick();   // SDRAM backpressure (init too)
    idle();
}

static uint8_t io_read(uint16_t a) {
    top->cpu_a = a; top->iorq_n = 0; top->rd_n = 0;
    tick(4);
    int guard = 100000;
    while (!top->cpu_wait_n && guard--) tick();   // honour WAIT stretching
    tick(2);
    uint8_t d = top->cpu_di;
    idle();
    return d;
}

static void io_write(uint16_t a, uint8_t d) {
    top->cpu_a = a; top->cpu_do = d; top->iorq_n = 0; top->wr_n = 0;
    tick(4);
    int guard = 100000;
    while (!top->cpu_wait_n && guard--) tick();
    tick(2);
    idle();
}

// Must match the generators in the Makefile.
static uint8_t test_rom_byte(int i) { return (uint8_t)((i * 7 + 3) & 0xFF); }
static uint8_t esx_rom_byte(int i)  { return (uint8_t)((i * 11 + 5) & 0xFF); }
static uint8_t rom128_byte(int i)   { return (uint8_t)((i * 13 + 7) & 0xFF); }

// An M1 opcode fetch: the automapper's trigger. Returns the fetched byte
// (which, per divMMC semantics, comes from the memory mapped BEFORE the
// fetch -- the map changes at the cycle's end).
static uint8_t m1_fetch(uint16_t a) {
    top->cpu_a = a; top->m1_n = 0; top->mreq_n = 0; top->rd_n = 0;
    tick(4);
    uint8_t d = top->cpu_di;
    idle();
    return d;
}

// key_matrix bit for half-row `row` (0 = A8 .. 7 = A15), key `bit` (0..4)
static uint64_t key(int row, int bit) { return 1ULL << (row * 5 + bit); }

int main(int argc, char** argv) {
    VerilatedContext ctx;
    ctx.commandArgs(argc, argv);
    top = new Vspeccy_tb_top(&ctx);
    sdram = new SdramModel();

    top->rst = 1; top->key_matrix = 0; top->joy_state = 0; top->ear_in = 1;
    top->cpu_a = 0; top->cpu_do = 0; top->divmmc_en = 0; top->en_128 = 0;
    top->dram_dq_in = 0xFFFF;
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

    // ---- AY on the 128K ports -------------------------------------------
    printf("AY\n");
    io_write(0xFFFD, 7);                      // select mixer
    io_write(0xBFFD, 0x38);
    check("R7 readback",        io_read(0xFFFD), 0x38);
    io_write(0xFFFD, 1);                      // coarse period: 4-bit mask
    io_write(0xBFFD, 0xFF);
    check("R1 masked to 4 bits", io_read(0xFFFD), 0x0F);
    io_write(0xFFFD, 14);                     // port A must exist + read back
    io_write(0xBFFD, 0x5A);
    check("R14 port A readback", io_read(0xFFFD), 0x5A);
    // 0x00FD has A15=0: not the AY, must still float.
    check("0x00FD still floats", io_read(0x00FD), 0xFF);

    // The machine must actually make sound. First a static level: volume 15
    // with the tone gate disabled must pin the audio bus at full scale --
    // this checks the DAC/mix path with no clocking involved.
    io_write(0xFFFD, 7);  io_write(0xBFFD, 0x3F);   // everything gated off
    io_write(0xFFFD, 8);  io_write(0xBFFD, 15);     // full fixed volume
    tick(16);
    check("static full volume on bus", top->audio, 255);

    // Then a real tone on channel A: the bus must move. R1 still holds 0x0F
    // from the mask test above -- clear it, or the period is 3860 and the
    // "silence" is the chip faithfully playing 34 Hz (been there).
    io_write(0xFFFD, 7);  io_write(0xBFFD, 0x3E);   // tone A only
    io_write(0xFFFD, 1);  io_write(0xBFFD, 0);      // coarse period clear!
    io_write(0xFFFD, 0);  io_write(0xBFFD, 20);     // period 20
    {
        int changes = 0, last = top->audio;
        for (int i = 0; i < 40000; i++) {
            tick();
            if (top->audio != last) { changes++; last = top->audio; }
        }
        check("AY tone reaches audio bus", changes > 10 ? 1 : 0, 1);
    }
    io_write(0xFFFD, 8);  io_write(0xBFFD, 0);      // silence it again

    // ---- divMMC ----------------------------------------------------------
    printf("divMMC\n");
    top->divmmc_en = 1;

    // Unmapped: low memory is the Spectrum ROM, ports quiet.
    check("unmapped: ROM at 0x0001",  mem_read(0x0001), test_rom_byte(1));

    // Entry trap is DELAYED: the trapped fetch itself reads Spectrum ROM...
    check("trap fetch sees old ROM",  m1_fetch(0x0000), test_rom_byte(0));
    // ...and afterwards esxDOS is mapped at 0x0000-0x1FFF.
    check("mapped: esx at 0x0001",    mem_read(0x0001), esx_rom_byte(1));
    check("mapped: esx at 0x1234",    mem_read(0x1234), esx_rom_byte(0x1234));
    check("0x4000+ unaffected",       mem_read(0x8000), 0x11);

    // Banked RAM window at 0x2000.
    io_write(0x00E3, 0x02);                       // bank 2
    mem_write(0x2000, 0xA7);
    check("divRAM bank2 write/read",  mem_read(0x2000), 0xA7);
    io_write(0x00E3, 0x00);                       // bank 0
    mem_write(0x2000, 0x11);
    check("divRAM bank0 distinct",    mem_read(0x2000), 0x11);
    io_write(0x00E3, 0x02);
    check("bank2 survives switch",    mem_read(0x2000), 0xA7);
    io_write(0x00E3, 0x04);                       // the bank esxDOS demands
    mem_write(0x2000, 0x44);
    check("bank4 exists",             mem_read(0x2000), 0x44);
    io_write(0x00E3, 0x00);
    check("bank4 distinct from 0",    mem_read(0x2000), 0x11);
    io_write(0x00E3, 0x02);

    // Exit trap (also delayed): RET at 0x1FF8 still executes from esx ROM.
    check("exit fetch sees esx",      m1_fetch(0x1FF8), esx_rom_byte(0x1FF8));
    check("unmapped again",           mem_read(0x0001), test_rom_byte(1));

    // CONMEM: manual mapping without any fetch.
    io_write(0x00E3, 0x80);
    check("CONMEM maps",              mem_read(0x0001), esx_rom_byte(1));
    io_write(0x00E3, 0x00);
    check("CONMEM clears",            mem_read(0x0001), test_rom_byte(1));

    // 0x3Dxx traps INSTANTLY: the fetch itself must read the divRAM window
    // (TR-DOS emulation / esxDOS trampoline). Seed a marker via CONMEM
    // first, then fetch it back with everything unmapped. Must run before
    // MAPRAM, which stickily replaces the esx ROM view.
    io_write(0x00E3, 0x80);                       // CONMEM, bank 0
    mem_write(0x3D42, 0xC9);                      // plant a marker
    io_write(0x00E3, 0x00);                       // fully unmapped again
    check("pre-3D: really unmapped",  mem_read(0x0001), test_rom_byte(1));
    check("0x3Dxx fetch is instant",  m1_fetch(0x3D42), 0xC9);
    check("0x3Dxx left map armed",    mem_read(0x0001), esx_rom_byte(1));
    m1_fetch(0x1FFF);                             // unmap

    // MAPRAM: bank 3 becomes the write-protected 'ROM'.
    io_write(0x00E3, 0x83);                       // CONMEM + bank 3
    mem_write(0x2000, 0x77);                      // seed bank 3
    io_write(0x00E3, 0xC0);                       // CONMEM + MAPRAM
    check("MAPRAM: bank3 at 0x0000",  mem_read(0x0000), 0x77);
    mem_write(0x0000, 0x55);
    check("MAPRAM: low is RO",        mem_read(0x0000), 0x77);
    io_write(0x00E3, 0xC3);                       // window also bank 3
    mem_write(0x2000, 0x99);
    check("MAPRAM: bank3 RO via win", mem_read(0x2000), 0x77);
    check("MAPRAM is sticky",         mem_read(0x0000), 0x77);
    io_write(0x00E3, 0x00);                       // cannot clear mapram...
    io_write(0x00E3, 0x80);
    check("...even via port",         mem_read(0x0000), 0x77);
    io_write(0x00E3, 0x00);

    // SPI: CS register and a loopback exchange (tb ties MISO to MOSI).
    check("SD /CS idle high",         top->sd_cs, 1);
    io_write(0x00E7, 0xFE);
    check("SD /CS asserted",          top->sd_cs, 0);
    io_write(0x00EB, 0xA5);
    check("SPI loopback",             io_read(0x00EB), 0xA5);
    io_write(0x00EB, 0x3C);
    check("SPI second exchange",      io_read(0x00EB), 0x3C);
    io_write(0x00E7, 0xFF);
    check("SD /CS released",          top->sd_cs, 1);

    // Disabled: no trap, ports dead.
    top->divmmc_en = 0;
    m1_fetch(0x0000);
    check("disabled: no trap",        mem_read(0x0001), test_rom_byte(1));
    top->divmmc_en = 1;

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

    // ---- 128K: #7FFD paging, twin ROMs, screens in M9K, banks in SDRAM ---
    printf("128K\n");
    top->divmmc_en = 0;
    top->en_128 = 1;
    top->rst = 1; idle(); tick(16); top->rst = 0;
    // No init wait on purpose: the first accesses below arrive while the
    // SDRAM controller is still initialising, exactly like the 128 ROM's
    // RAM test on real hardware -- WAIT must carry the CPU through it.
    tick(32);

    // ROM select: reset state is ROM0, the 128 editor.
    check("ROM0 (128 editor) mapped",  mem_read(0x0001), rom128_byte(1));
    io_write(0x7FFD, 0x10);
    check("bit4 pages in ROM1 (48)",   mem_read(0x0001), test_rom_byte(1));
    io_write(0x7FFD, 0x00);
    check("and back to ROM0",          mem_read(0x0001), rom128_byte(1));

    // Eight banks at 0xC000, all distinct (5/7 in M9K, the rest in SDRAM).
    for (int b = 0; b < 8; b++) {
        io_write(0x7FFD, (uint8_t)b);
        mem_write((uint16_t)(0xC000 + b), (uint8_t)(0xB0 + b));
    }
    {
        int wrong = 0;
        for (int b = 0; b < 8; b++) {
            io_write(0x7FFD, (uint8_t)b);
            if (mem_read((uint16_t)(0xC000 + b)) != (uint8_t)(0xB0 + b)) wrong++;
        }
        check("8 banks hold distinct data", wrong, 0);
    }

    // Bank 5 at 0xC000 is the same memory as the screen at 0x4000.
    io_write(0x7FFD, 5);
    mem_write(0xC123, 0x77);
    check("bank5 C123 == 4123",        mem_read(0x4123), 0x77);
    mem_write(0x4321, 0x66);
    check("4321 == bank5 C321",        mem_read(0xC321), 0x66);

    // 0x8000 is always bank 2.
    io_write(0x7FFD, 2);
    mem_write(0x8055, 0x22);
    check("bank2 8055 == C055",        mem_read(0xC055), 0x22);

    // Shadow screen: fill bank 5's attributes with white paper and bank 7's
    // with red, then flip #7FFD bit 3 and watch the green channel: white
    // paper has green, red paper has none. Border black so it stays out.
    io_write(0x00FE, 0x00);
    io_write(0x7FFD, 0x05);                        // bank 5 for good measure
    for (int i = 0; i < 768; i++) mem_write((uint16_t)(0x5800 + i), 0x38);
    io_write(0x7FFD, 0x07);                        // bank 7 at 0xC000
    for (int i = 0; i < 768; i++) mem_write((uint16_t)(0xD800 + i), 0x10);
    {
        auto frame_has_green = [&]() {
            long g = 0;
            for (long i = 0; i < 279552; i++) { tick(); if (top->vga_g) g++; }
            return g > 1000;
        };
        io_write(0x7FFD, 0x00);                    // screen 5: white
        check("screen 5 shows white",  frame_has_green() ? 1 : 0, 1);
        io_write(0x7FFD, 0x08);                    // screen 7: red
        check("screen 7 shows red",    frame_has_green() ? 1 : 0, 0);
        io_write(0x7FFD, 0x00);
        check("back to screen 5",      frame_has_green() ? 1 : 0, 1);
    }


    // Lock bit: further writes ignored until reset.
    io_write(0x7FFD, 0x20 | 3);
    mem_write(0xC077, 0x33);
    io_write(0x7FFD, 0x00);                        // must bounce off the lock
    check("lock holds bank 3",         mem_read(0xC077), 0x33);
    check("lock holds ROM0",           mem_read(0x0001), rom128_byte(1));

    check("no SDRAM protocol errors",  sdram->errors, 0);

    // 48K regression: a reset without en_128 is the classic machine.
    top->en_128 = 0;
    top->rst = 1; idle(); tick(16); top->rst = 0; tick(32);
    io_write(0x7FFD, 5);                           // must be inert
    mem_write(0xC200, 0x55);                       // lands in ramhi
    check("48K: 7FFD inert, ramhi lives", mem_read(0xC200), 0x55);
    check("48K: the 48 ROM",           mem_read(0x0001), test_rom_byte(1));

    top->final();
    delete top;
    printf(failures ? "FAILED (%d)\n" : "ok\n", failures);
    return failures ? 1 : 0;
}
