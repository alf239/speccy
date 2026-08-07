// ---------------------------------------------------------------------------
// Boot test: run the full machine and watch it like a monitor.
//
// Unlike the other testbenches this one has no debug taps into the design.
// It reconstructs the picture purely from the VGA outputs -- locking onto
// hsync and vsync edges exactly as a real display would -- so it exercises
// the one thing the frame-dump harness cannot: whether the sync stream
// itself makes sense.
//
//   boot_tb [--frames N] [--out FILE.bmp] [--all] [--expect-smoke]
//           [--type "STRING"] [--type-at FRAME] [--wav FILE.wav]
//
// --all dumps every frame (boot animation); default dumps only the last.
// --expect-smoke asserts on the smoke-test ROM's known output, making this
// runnable as a regression test without any copyrighted ROM present.
// --type drives the string into the PS/2 pins as real scancode traffic once
// the machine has booted (--type-at, default frame 120). '\n' is Enter;
// '"' uses the SYM+P chord via the quote scancode.
// ---------------------------------------------------------------------------

#include <verilated.h>
#include "Vboot_tb_top.h"
#include "sd_model.h"
#include "sdram_model.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>

static const int W = 448, H = 624;   // one scandoubled frame

// ---------------------------------------------------------------------------
static void put32(uint8_t* p, uint32_t v) {
    p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF;
}

static bool write_bmp(const char* path, const uint8_t* rgb, int w, int h) {
    const int row_bytes = w * 3, pad = (4 - (row_bytes % 4)) % 4;
    const int data_size = (row_bytes + pad) * h;
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    uint8_t hdr[54]; memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'B'; hdr[1] = 'M';
    put32(hdr + 2, 54 + data_size); put32(hdr + 10, 54); put32(hdr + 14, 40);
    put32(hdr + 18, (uint32_t)w);   put32(hdr + 22, (uint32_t)h);
    hdr[26] = 1; hdr[28] = 24;      put32(hdr + 34, (uint32_t)data_size);
    fwrite(hdr, 1, sizeof(hdr), f);
    const uint8_t zero[3] = {0, 0, 0};
    for (int y = h - 1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            const uint8_t* p = rgb + (y * w + x) * 3;
            const uint8_t bgr[3] = { p[2], p[1], p[0] };
            fwrite(bgr, 1, 3, f);
        }
        if (pad) fwrite(zero, 1, (size_t)pad, f);
    }
    fclose(f);
    return true;
}

static inline uint8_t expand4(uint8_t v) { return (uint8_t)(v * 17); }

// ---------------------------------------------------------------------------
// PS/2 typist: turns a string into scancode traffic, clocked out on the PS/2
// pins at ~12.5 kHz while the machine runs. Each key is held for 3 frames and
// released for 1 -- comfortably longer than the ROM's per-frame debounce.
// ---------------------------------------------------------------------------
struct Typist {
    std::vector<uint8_t> bytes;      // flat stream: makes, breaks, gaps
    enum : uint8_t { GAP = 0x00 };   // pseudo-byte: pause ~3 frames

    size_t  idx = 0;
    int     bit = -1;                // -1: idle/gap
    long    timer = 0;
    std::vector<int> frame;          // 11 bits of the current byte

    static constexpr long HALF   = 560;    // 12.5 kHz at 14 MHz
    // Release gap must exceed the ROM's ~5-frame KSTATE recovery, or the
    // second press of a doubled letter ("ll" in hello) is swallowed.
    static constexpr long GAPLEN = 6L * 279552;

    static uint8_t sc(char c) {            // char -> set-2 make code
        static const struct { char ch; uint8_t code; } tab[] = {
            {'a',0x1C},{'b',0x32},{'c',0x21},{'d',0x23},{'e',0x24},{'f',0x2B},
            {'g',0x34},{'h',0x33},{'i',0x43},{'j',0x3B},{'k',0x42},{'l',0x4B},
            {'m',0x3A},{'n',0x31},{'o',0x44},{'p',0x4D},{'q',0x15},{'r',0x2D},
            {'s',0x1B},{'t',0x2C},{'u',0x3C},{'v',0x2A},{'w',0x1D},{'x',0x22},
            {'y',0x35},{'z',0x1A},
            {'1',0x16},{'2',0x1E},{'3',0x26},{'4',0x25},{'5',0x2E},{'6',0x36},
            {'7',0x3D},{'8',0x3E},{'9',0x46},{'0',0x45},
            {' ',0x29},{'\n',0x5A},{'"',0x52},{',',0x41},{'.',0x49},
            {'-',0x4E},{'=',0x55},{';',0x4C},{'/',0x4A},
        };
        for (auto& t : tab) if (t.ch == c) return t.code;
        return 0;
    }

    void program(const std::string& text) {
        for (char raw : text) {
            const char c = (raw >= 'A' && raw <= 'Z') ? (char)(raw - 'A' + 'a') : raw;
            const uint8_t k = sc(c);
            if (!k) { fprintf(stderr, "typist: no scancode for '%c', skipped\n", raw); continue; }
            bytes.push_back(k);                 // make
            bytes.push_back(GAP);               // hold
            bytes.push_back(0xF0);              // break
            bytes.push_back(k);
            bytes.push_back(GAP);               // release gap
        }
    }

    bool done() const { return idx >= bytes.size() && bit < 0; }

    // Advance one 14 MHz clock; returns {clk, data} to drive.
    void step(uint8_t& pclk, uint8_t& pdat) {
        pclk = 1; pdat = 1;
        if (bit < 0) {                          // between bytes
            if (timer > 0) { timer--; return; }
            while (idx < bytes.size() && bytes[idx] == GAP) { timer = GAPLEN; idx++; return; }
            if (idx >= bytes.size()) return;
            const uint8_t b = bytes[idx++];
            int ones = 0;
            for (int i = 0; i < 8; i++) ones += (b >> i) & 1;
            frame.clear();
            frame.push_back(0);
            for (int i = 0; i < 8; i++) frame.push_back((b >> i) & 1);
            frame.push_back((ones & 1) ? 0 : 1);
            frame.push_back(1);
            bit = 0; timer = 0;
        }
        // Drive the current bit: data stable, clock low in the middle half.
        const long ph = timer % (2 * HALF);
        pdat = (uint8_t)frame[(size_t)bit];
        pclk = (ph >= HALF / 2 && ph < HALF / 2 + HALF) ? 0 : 1;
        timer++;
        if (timer >= 2 * HALF) {
            timer = 0;
            bit++;
            if (bit >= (int)frame.size()) { bit = -1; timer = HALF; }
        }
    }
};

int main(int argc, char** argv) {
    int         frames = 8;
    bool        all = false, expect_smoke = false;
    std::string outfile = "out/boot.bmp";

    std::string type_str;
    std::string wav_path;
    int type_at = 120;
    bool snap = false, expect_snap = false;
    int  snap_at = -1;                 // re-arm + reset at this frame
    std::string sd_path;
    bool esx = false;
    int  nmi_at = -1;
    int  reset_at = -1;                // plain KEY[0]-style reset at this frame
    bool m128 = false;                 // boot the 128K machine

    for (int i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--out")    && i + 1 < argc) outfile = argv[++i];
        else if (!strcmp(argv[i], "--type")   && i + 1 < argc) type_str = argv[++i];
        else if (!strcmp(argv[i], "--wav")    && i + 1 < argc) wav_path = argv[++i];
        else if (!strcmp(argv[i], "--type-at")&& i + 1 < argc) type_at = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--all"))                    all = true;
        else if (!strcmp(argv[i], "--snap"))                   snap = true;
        else if (!strcmp(argv[i], "--snap-at") && i + 1 < argc) snap_at = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--sd")     && i + 1 < argc) sd_path = argv[++i];
        else if (!strcmp(argv[i], "--nmi-at") && i + 1 < argc) nmi_at = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--reset-at") && i + 1 < argc) reset_at = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--128"))                    m128 = true;
        else if (!strcmp(argv[i], "--esx"))                    esx = true;
        else if (!strcmp(argv[i], "--sdlog"))                  { /* set below */ }
        else if (!strcmp(argv[i], "--expect-snap"))            { snap = true; expect_snap = true; }
        else if (!strcmp(argv[i], "--expect-smoke"))           expect_smoke = true;
    }

    VerilatedContext ctx;
    ctx.commandArgs(argc, argv);
    Vboot_tb_top top(&ctx);

    top.rst = 1; top.key_matrix = 0; top.joy_state = 0; top.ear_in = 1;
    top.ps2_clk = 1; top.ps2_data = 1;
    top.arm_snapshot = snap ? 1 : 0;
    top.divmmc_en = esx ? 1 : 0;
    top.en_128 = m128 ? 1 : 0;
    top.nmi_button = 0;
    top.sd_miso = 1;
    top.dram_dq_in = 0xFFFF;
    SdramModel sdram_chip;

    SdModel* card = nullptr;
    if (!sd_path.empty()) {
        FILE* f = fopen(sd_path.c_str(), "rb");
        if (!f) { fprintf(stderr, "cannot open %s\n", sd_path.c_str()); return 1; }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        card = new SdModel((size_t)(sz + 511) / 512);
        if (fread(card->disk.data(), 1, sz, f) != (size_t)sz) { fprintf(stderr, "short read\n"); return 1; }
        fclose(f);
        printf("SD image: %s (%ld bytes)\n", sd_path.c_str(), sz);
        for (int i = 1; i < argc; i++)
            if (!strcmp(argv[i], "--sdlog")) card->verbose = true;
    }
    for (int i = 0; i < 16; i++) { top.clk = 0; top.eval(); top.clk = 1; top.eval(); }
    top.rst = 0;

    // ---- the virtual monitor ---------------------------------------------
    // hpos: clocks since the last hsync leading edge. vpos: hsyncs since the
    // last vsync leading edge. That is all a monitor knows.
    std::vector<uint8_t> fb((size_t)W * H * 3, 0);
    int  hpos = 0, vpos = 0, frame_no = 0;
    bool prev_hs = false, prev_vs = false;
    long hs_edges = 0, vs_edges = 0;
    long lines_this_frame = 0, lines_last_frame = 0;

    Typist typist;
    if (!type_str.empty()) typist.program(type_str);
    bool typing_started = false;

    // Speaker capture: sample every 320 clks -> 43750 Hz, 8-bit mono.
    std::vector<uint8_t> audio;
    int wav_div = 0;

    while (frame_no < frames) {
        if (typing_started && !typist.done()) {
            uint8_t pc, pd;
            typist.step(pc, pd);
            top.ps2_clk = pc; top.ps2_data = pd;
        }
        top.clk = 0; top.eval();
        top.clk = 1; top.eval();
        if (card) top.sd_miso = card->step(top.sd_cs, top.sd_sck, top.sd_mosi) ? 1 : 0;
        {
            uint16_t dq = 0xFFFF;
            sdram_chip.step(top.dram_cs_n, top.dram_ras_n, top.dram_cas_n,
                            top.dram_we_n, top.dram_addr, top.dram_ba,
                            top.dram_ldqm, top.dram_udqm,
                            top.dram_dq_out, top.dram_dq_oe, dq);
            top.dram_dq_in = dq;
        }

        if (!wav_path.empty() && ++wav_div == 320) {
            wav_div = 0;
            // 10-bit beeper+AY mix from the machine, recorded as 8-bit PCM.
            audio.push_back((uint8_t)(top.audio >> 2));
        }

        const bool hs = top.vga_hsync, vs = top.vga_vsync;

        if (hs && !prev_hs) {                    // new line
            hpos = 0;
            vpos++;
            hs_edges++;
            lines_this_frame++;
        }
        if (vs && !prev_vs) {                    // new frame
            vs_edges++;
            if (vs_edges > 1) {                  // first edge = start of frame 0
                if (all || frame_no == frames - 1) {
                    char path[512];
                    if (all) snprintf(path, sizeof(path), "out/boot%03d.bmp", frame_no);
                    else     snprintf(path, sizeof(path), "%s", outfile.c_str());
                    write_bmp(path, fb.data(), W, H);
                }
                frame_no++;
                if (nmi_at >= 0 && frame_no == nmi_at)     top.nmi_button = 1;
                if (nmi_at >= 0 && frame_no == nmi_at + 2) top.nmi_button = 0;
                if (frame_no == reset_at) {
                    // Soft reset mid-run: what KEY[0] does on the board. The
                    // machine's block RAMs keep their contents, exactly like
                    // hardware between configurations.
                    printf("  reset at frame %d\n", frame_no);
                    top.rst = 1;
                    for (int k = 0; k < 16; k++) { top.clk = 0; top.eval(); top.clk = 1; top.eval(); }
                    top.rst = 0;
                }
                if (frame_no == snap_at) {
                    // The bug this guards: block RAM init is configuration-
                    // time only, so an armed reset must REFILL RAM from the
                    // shadow, not resume registers over stale memory.
                    printf("  re-arming snapshot + reset at frame %d\n", frame_no);
                    top.arm_snapshot = 1;
                    top.rst = 1;
                    for (int k = 0; k < 16; k++) { top.clk = 0; top.eval(); top.clk = 1; top.eval(); }
                    top.rst = 0;
                }
                if (!type_str.empty() && frame_no == type_at) {
                    typing_started = true;
                    printf("  typing \"%s\" from frame %d\n", type_str.c_str(), type_at);
                }
                if ((frame_no % 25) == 0)
                    printf("  frame %d/%d\n", frame_no, frames);
            }
            vpos = 0;
            lines_last_frame = lines_this_frame;
            lines_this_frame = 0;
        }
        prev_hs = hs; prev_vs = vs;

        if (hpos < W && vpos < H) {
            uint8_t* p = fb.data() + ((size_t)vpos * W + hpos) * 3;
            p[0] = expand4(top.vga_r);
            p[1] = expand4(top.vga_g);
            p[2] = expand4(top.vga_b);
        }
        hpos++;
    }

    // Sanity on the sync stream itself: 624 hsyncs between vsync edges.
    // (Total/total would fold in the partial frame before the first vsync.)
    printf("saw %ld hsync and %ld vsync edges; last complete frame had %ld lines (expect 624)\n",
           hs_edges, vs_edges, lines_last_frame);

    int failures = 0;
    if (lines_last_frame != 624) {
        fprintf(stderr, "FAIL: %ld lines in the last frame, expected 624\n", lines_last_frame);
        failures++;
    }

    if (expect_snap) {
        // The synthetic snapshot's stub sets border 5; its program then sets
        // border 2, writes HL=0xAAAA to 0x4000 (bright white ink there via
        // the snapshot's attribute byte) and loops. Border 2 alone proves the
        // full chain: overlay served, registers restored, JP taken, program
        // running from snapshot RAM.
        long white = 0;
        for (size_t i = 0; i < fb.size(); i += 3)
            if (fb[i] == 255 && fb[i+1] == 255 && fb[i+2] == 255) white++;
        printf("snap check: border=%d (expect 2), %ld bright-white pixels (expect >=16)\n",
               top.border, white);
        if (top.border != 2) { fprintf(stderr, "FAIL: border %d\n", top.border); failures++; }
        if (white < 16)      { fprintf(stderr, "FAIL: snapshot bitmap not visible\n"); failures++; }
    }

    if (expect_smoke) {
        // The smoke ROM sets border red and writes one 0xAA bitmap byte with
        // attribute 0x55 (bright magenta paper... no: paper 2=red, ink 5=cyan,
        // bright 1). Just count: a red-border frame is mostly (204,0,0).
        long red = 0, other_nonblack = 0;
        for (size_t i = 0; i < fb.size(); i += 3) {
            const bool r = fb[i] > 0, g = fb[i+1] > 0, b = fb[i+2] > 0;
            if (r && !g && !b) red++;
            else if (r || g || b) other_nonblack++;
        }
        printf("smoke check: %ld red pixels, %ld other non-black\n", red, other_nonblack);
        // Border is most of the frame; the 8-pixel 0xAA row contributes a few
        // cyan pixels (doubled = a handful). Loose bounds, deliberately.
        if (red < 100000) { fprintf(stderr, "FAIL: expected a red border\n"); failures++; }
        if (other_nonblack < 4) { fprintf(stderr, "FAIL: expected the written screen byte\n"); failures++; }
    }

    if (!wav_path.empty()) {
        FILE* wf = fopen(wav_path.c_str(), "wb");
        if (wf) {
            const uint32_t rate = 43750, dsz = (uint32_t)audio.size();
            uint8_t h[44] = {'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',
                             16,0,0,0, 1,0, 1,0, 0,0,0,0, 0,0,0,0, 1,0, 8,0,
                             'd','a','t','a',0,0,0,0};
            uint32_t riff = 36 + dsz;
            memcpy(h+4,  &riff, 4);
            memcpy(h+24, &rate, 4);
            memcpy(h+28, &rate, 4);   // byte rate = rate * 1ch * 1B
            memcpy(h+40, &dsz,  4);
            fwrite(h, 1, 44, wf);
            fwrite(audio.data(), 1, dsz, wf);
            fclose(wf);
            printf("wrote %s: %.1f s of beeper\n", wav_path.c_str(), dsz/(double)rate);
        }
    }

    top.final();
    printf(failures ? "FAILED (%d)\n" : "ok\n", failures);
    return failures ? 1 : 0;
}
