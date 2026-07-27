// ---------------------------------------------------------------------------
// Stage 0/1 harness: run the video generator through the scandoubler and dump
// both rasters as BMP, checking one against the other.
//
// No CPU yet. The screen bank is loaded through the RAM's CPU-side port (the
// same port a Z80 will eventually use), then the raster runs free.
//
// Two frames are captured each time round:
//   in_frameNN.bmp    448x312  the 15.625 kHz Spectrum raster
//   out_frameNN.bmp   448x624  the 31.25 kHz signal a monitor would receive
//
// and three things are checked:
//   * exactly 448*624 pixels per output frame
//   * output lines 2k and 2k+1 are identical -- the scandoubler's whole job
//   * the set of doubled lines matches the set of input lines
//
// BMP rather than PNG purely because it needs no libraries: a 54-byte header
// and raw BGR. Quick Look and Preview both open them.
//
// Usage:
//   speccy_video_sim [--frames N] [--out DIR] [--scr FILE.scr]
//                    [--border N] [--cycle]
// ---------------------------------------------------------------------------

#include <verilated.h>
#include "Vspeccy_video_top.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>

static const int IN_W  = 448, IN_H  = 312;   // 15.625 kHz raster
static const int OUT_W = 448, OUT_H = 624;   // 31.25 kHz raster

static const int SCREEN_BYTES = 6912;        // 6144 bitmap + 768 attribute

// ---------------------------------------------------------------------------
// Screen address arithmetic (mirrors rtl/video.v)
// ---------------------------------------------------------------------------
static int bitmap_offset(int cx, int y) {
    return ((y & 0xC0) << 5) | ((y & 0x07) << 8) | ((y & 0x38) << 2) | cx;
}

static int attr_offset(int cx, int cy) {
    return 0x1800 + cy * 32 + cx;
}

static void make_test_pattern(uint8_t* scr) {
    memset(scr, 0, SCREEN_BYTES);

    for (int y = 0; y < 192; y++) {
        for (int cx = 0; cx < 32; cx++) {
            uint8_t b;
            switch (y / 48) {
                case 0:  b = 0xAA; break;   // 1-pixel stripes -- proves the pixel clock
                case 1:  b = 0xCC; break;   // 2-pixel
                case 2:  b = 0xF0; break;   // 4-pixel
                default: b = (uint8_t)(1u << (7 - ((cx + (y >> 3)) & 7))); break;
            }
            // One-pixel frame around the display window, so its exact edges
            // can be checked against the timing parameters.
            if (y == 0 || y == 191) b = 0xFF;
            if (cx == 0)            b |= 0x80;
            if (cx == 31)           b |= 0x01;

            scr[bitmap_offset(cx, y)] = b;
        }
    }

    for (int cy = 0; cy < 24; cy++) {
        for (int cx = 0; cx < 32; cx++) {
            int paper  = (cx / 4) & 7;
            int ink    = 7 - paper;          // always contrasting
            int bright = (cy / 8) == 1;
            int flash  = (cy / 8) == 2;
            scr[attr_offset(cx, cy)] =
                (uint8_t)((flash << 7) | (bright << 6) | (paper << 3) | ink);
        }
    }
}

// ---------------------------------------------------------------------------
// BMP output
// ---------------------------------------------------------------------------
static void put32(uint8_t* p, uint32_t v) {
    p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF;
}

static bool write_bmp(const char* path, const uint8_t* rgb, int w, int h) {
    const int row_bytes = w * 3;
    const int pad       = (4 - (row_bytes % 4)) % 4;
    const int data_size = (row_bytes + pad) * h;

    FILE* f = fopen(path, "wb");
    if (!f) return false;

    uint8_t hdr[54];
    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'B'; hdr[1] = 'M';
    put32(hdr + 2,  54 + data_size);
    put32(hdr + 10, 54);
    put32(hdr + 14, 40);
    put32(hdr + 18, (uint32_t)w);
    put32(hdr + 22, (uint32_t)h);
    hdr[26] = 1;    // planes
    hdr[28] = 24;   // bits per pixel
    put32(hdr + 34, (uint32_t)data_size);
    fwrite(hdr, 1, sizeof(hdr), f);

    const uint8_t zero[3] = {0, 0, 0};
    for (int y = h - 1; y >= 0; y--) {          // BMP rows run bottom-up
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

// 4-bit channel -> 8-bit: 0x0 -> 0, 0xC -> 204, 0xF -> 255
static inline uint8_t expand4(uint8_t v) { return (uint8_t)(v * 17); }

// Mirrors rtl/palette.v, for expanding the pre-scandoubler index stream.
static void idx_to_rgb(uint8_t idx, bool blank, uint8_t* out) {
    const uint8_t level = (idx & 0x8) ? 0xF : 0xC;
    out[0] = (blank || !(idx & 0x2)) ? 0 : expand4(level);   // red
    out[1] = (blank || !(idx & 0x4)) ? 0 : expand4(level);   // green
    out[2] = (blank || !(idx & 0x1)) ? 0 : expand4(level);   // blue
}

// ---------------------------------------------------------------------------
// Memory images for synthesis.
//
// Emitted from the same screen the simulation runs, so the board and the
// testbench cannot drift apart. .hex feeds $readmemh (which both Verilator and
// Quartus accept); .mif is the fallback if Quartus declines to infer an
// initialised RAM from $readmemh.
// ---------------------------------------------------------------------------
static const int VRAM_BYTES = 16384;

static bool write_mem_images(const std::string& prefix, const uint8_t* scr) {
    std::vector<uint8_t> bank(VRAM_BYTES, 0);
    memcpy(bank.data(), scr, SCREEN_BYTES);

    const std::string hexpath = prefix + ".hex";
    FILE* f = fopen(hexpath.c_str(), "w");
    if (!f) return false;
    for (int i = 0; i < VRAM_BYTES; i++) fprintf(f, "%02X\n", bank[i]);
    fclose(f);

    const std::string mifpath = prefix + ".mif";
    f = fopen(mifpath.c_str(), "w");
    if (!f) return false;
    fprintf(f, "DEPTH = %d;\nWIDTH = 8;\n"
               "ADDRESS_RADIX = HEX;\nDATA_RADIX = HEX;\nCONTENT BEGIN\n", VRAM_BYTES);
    for (int i = 0; i < VRAM_BYTES; i++) fprintf(f, "  %04X : %02X;\n", i, bank[i]);
    fprintf(f, "END;\n");
    fclose(f);

    printf("wrote %s and %s (%d bytes of screen in a %d byte bank)\n",
           hexpath.c_str(), mifpath.c_str(), SCREEN_BYTES, VRAM_BYTES);
    return true;
}

// FNV-1a over one row of a framebuffer
static uint64_t line_hash(const uint8_t* fb, int w, int y) {
    uint64_t h = 1469598103934665603ULL;
    const uint8_t* p = fb + (size_t)y * w * 3;
    for (int i = 0; i < w * 3; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    int         frames  = 2;
    int         border  = 2;      // red
    bool        cycle   = false;
    std::string outdir  = "out";
    std::string scrfile;
    std::string memprefix;

    for (int i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "--frames") && i + 1 < argc) frames  = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--border") && i + 1 < argc) border  = atoi(argv[++i]) & 7;
        else if (!strcmp(argv[i], "--out")    && i + 1 < argc) outdir  = argv[++i];
        else if (!strcmp(argv[i], "--scr")    && i + 1 < argc) scrfile = argv[++i];
        else if (!strcmp(argv[i], "--mem")    && i + 1 < argc) memprefix = argv[++i];
        else if (!strcmp(argv[i], "--cycle"))                  cycle   = true;
    }

    VerilatedContext ctx;
    ctx.commandArgs(argc, argv);
    Vspeccy_video_top top(&ctx);

    // ---- screen contents -------------------------------------------------
    std::vector<uint8_t> scr(SCREEN_BYTES, 0);
    if (!scrfile.empty()) {
        FILE* f = fopen(scrfile.c_str(), "rb");
        if (!f) { fprintf(stderr, "cannot open %s\n", scrfile.c_str()); return 1; }
        size_t n = fread(scr.data(), 1, SCREEN_BYTES, f);
        fclose(f);
        if (n != (size_t)SCREEN_BYTES) {
            fprintf(stderr, "%s: expected %d bytes, got %zu\n", scrfile.c_str(), SCREEN_BYTES, n);
            return 1;
        }
        printf("loaded %s\n", scrfile.c_str());
    } else {
        make_test_pattern(scr.data());
        printf("using synthetic test pattern\n");
    }

    if (!memprefix.empty() && !write_mem_images(memprefix, scr.data())) {
        fprintf(stderr, "cannot write memory images with prefix %s\n", memprefix.c_str());
        return 1;
    }

    std::vector<uint8_t> in_fb ((size_t)IN_W  * IN_H  * 3, 0);
    std::vector<uint8_t> out_fb((size_t)OUT_W * OUT_H * 3, 0);
    long in_px = 0, out_px = 0;

    // A pixel of the input stream spans two clk cycles (14 MHz clock, 7 MHz
    // pixel enable), so px_h/px_v hold their value across both -- end-of-frame
    // must be tested only on the cycle that actually produced a pixel.
    // The output side produces a pixel every clk.
    bool capture = false;

    auto tick = [&]() -> bool {
        top.clk = 0; top.eval();
        const bool ce = top.ce_pix;
        top.clk = 1; top.eval();

        if (capture) {
            const int oh = top.out_h, ov = top.out_v;
            if (oh < OUT_W && ov < OUT_H) {
                uint8_t* p = out_fb.data() + ((size_t)ov * OUT_W + oh) * 3;
                p[0] = expand4(top.vga_r);
                p[1] = expand4(top.vga_g);
                p[2] = expand4(top.vga_b);
                out_px++;
            }
            if (ce) {
                const int h = top.px_h, v = top.px_v;
                if (h < IN_W && v < IN_H) {
                    idx_to_rgb(top.px_idx, top.px_blank,
                               in_fb.data() + ((size_t)v * IN_W + h) * 3);
                    in_px++;
                }
            }
        }
        return ce;
    };

    auto at_out_frame_end = [&]() { return top.out_h == OUT_W - 1 && top.out_v == OUT_H - 1; };
    auto at_in_frame_end  = [&]() { return top.px_h  == IN_W  - 1 && top.px_v  == IN_H  - 1; };

    // ---- reset -----------------------------------------------------------
    top.rst = 1; top.cpu_we = 0; top.cpu_addr = 0; top.cpu_din = 0;
    top.border_colour = border;
    for (int i = 0; i < 8; i++) tick();
    top.rst = 0;

    // ---- load the screen through the CPU-side RAM port --------------------
    for (int i = 0; i < SCREEN_BYTES; i++) {
        top.cpu_addr = i;
        top.cpu_din  = scr[i];
        top.cpu_we   = 1;
        tick();
    }
    top.cpu_we = 0;

    // ---- warm up: discard one full input frame, then stop exactly at an
    //      output frame boundary so the line buffers hold real data and the
    //      next tick begins a fresh frame -------------------------------
    while (!(tick() && at_in_frame_end())) { }
    while (!at_out_frame_end()) tick();
    capture = true;
    in_px = out_px = 0;

    printf("capturing %d frames -> %s/  (border=%d%s)\n",
           frames, outdir.c_str(), border, cycle ? ", cycling" : "");

    int written = 0, failures = 0;
    while (written < frames) {
        if (cycle) top.border_colour = (border + written) & 7;

        // A frame is complete on the tick that emits its final pixel -- test
        // after that tick, never before, or the next frame's first pixel is
        // already in the buffer.
        const bool ce = tick();

        if (ce && at_in_frame_end()) {
            char path[512];
            snprintf(path, sizeof(path), "%s/in_frame%02d.bmp", outdir.c_str(), written);
            write_bmp(path, in_fb.data(), IN_W, IN_H);
            if (in_px != (long)IN_W * IN_H) {
                fprintf(stderr, "FAIL: input frame %d has %ld pixels, expected %d\n",
                        written, in_px, IN_W * IN_H);
                failures++;
            }
            in_px = 0;
        }

        if (!at_out_frame_end()) continue;

        // ---- checks ------------------------------------------------------
        if (out_px != (long)OUT_W * OUT_H) {
            fprintf(stderr, "FAIL: output frame %d has %ld pixels, expected %d\n",
                    written, out_px, OUT_W * OUT_H);
            failures++;
        }

        int bad_pairs = 0;
        std::string bad_list;
        std::vector<uint64_t> doubled, original;
        for (int k = 0; k < OUT_H / 2; k++) {
            const uint64_t a = line_hash(out_fb.data(), OUT_W, 2 * k);
            const uint64_t b = line_hash(out_fb.data(), OUT_W, 2 * k + 1);
            if (a != b) {
                bad_pairs++;
                if (bad_pairs <= 8) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), " %d", k);
                    bad_list += buf;
                }
            }
            doubled.push_back(a);
        }
        for (int y = 0; y < IN_H; y++)
            original.push_back(line_hash(in_fb.data(), IN_W, y));

        std::sort(doubled.begin(), doubled.end());
        std::sort(original.begin(), original.end());
        const bool same_lines = (doubled == original);

        printf("  frame %d: %ld out px, %d/%d line pairs identical, "
               "line set %s input\n",
               written, out_px, OUT_H / 2 - bad_pairs, OUT_H / 2,
               same_lines ? "matches" : "DIFFERS from");

        if (bad_pairs) {
            fprintf(stderr, "FAIL: %d output line pairs differ (first at:%s)\n",
                    bad_pairs, bad_list.c_str());
            failures++;
        }
        if (!same_lines && !cycle) { fprintf(stderr, "FAIL: doubled lines do not match input\n"); failures++; }

        char path[512];
        snprintf(path, sizeof(path), "%s/out_frame%02d.bmp", outdir.c_str(), written);
        if (!write_bmp(path, out_fb.data(), OUT_W, OUT_H)) {
            fprintf(stderr, "cannot write %s (does %s/ exist?)\n", path, outdir.c_str());
            return 1;
        }
        out_px = 0;
        written++;
    }

    top.final();
    printf(failures ? "FAILED (%d)\n" : "ok\n", failures);
    return failures ? 1 : 0;
}
