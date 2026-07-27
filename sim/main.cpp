// ---------------------------------------------------------------------------
// Stage 0 harness: run the video generator and dump frames as BMP.
//
// No CPU yet. The screen bank is loaded through the RAM's CPU-side port (the
// same port a Z80 will eventually use), then the raster runs free and every
// completed frame is written out.
//
// BMP rather than PNG purely because it needs no libraries: a 54-byte header
// and raw BGR. Quick Look and Preview both open them.
//
// Usage:
//   speccy_video_sim [--frames N] [--out DIR] [--scr FILE.scr]
//
// With no --scr, a synthetic test pattern is generated that exercises pixel
// detail, all eight colours, BRIGHT and FLASH.
// ---------------------------------------------------------------------------

#include <verilated.h>
#include "Vspeccy_video_top.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>

// Raster dimensions -- we capture the entire frame including blanking, so that
// sync placement and the exact edges of the display window are all visible.
static const int FRAME_W = 448;
static const int FRAME_H = 312;

static const int SCREEN_BYTES = 6912;   // 6144 bitmap + 768 attribute

// ---------------------------------------------------------------------------
// Screen address arithmetic (mirrors rtl/video.v)
// ---------------------------------------------------------------------------
static int bitmap_offset(int cx, int y) {
    return ((y & 0xC0) << 5) | ((y & 0x07) << 8) | ((y & 0x38) << 2) | cx;
}

static int attr_offset(int cx, int cy) {
    return 0x1800 + cy * 32 + cx;
}

// ---------------------------------------------------------------------------
// Test pattern
// ---------------------------------------------------------------------------
static void make_test_pattern(uint8_t* scr) {
    memset(scr, 0, SCREEN_BYTES);

    for (int y = 0; y < 192; y++) {
        for (int cx = 0; cx < 32; cx++) {
            uint8_t b;
            switch (y / 48) {
                case 0:  b = 0xAA; break;   // 1-pixel stripes -- proves the pixel clock
                case 1:  b = 0xCC; break;   // 2-pixel
                case 2:  b = 0xF0; break;   // 4-pixel
                default: b = (uint8_t)(1u << (7 - ((cx + (y >> 3)) & 7))); break;  // diagonal
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

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    int         frames  = 8;
    std::string outdir  = "out";
    std::string scrfile;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--frames") && i + 1 < argc)     frames  = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--out") && i + 1 < argc)   outdir  = argv[++i];
        else if (!strcmp(argv[i], "--scr") && i + 1 < argc)   scrfile = argv[++i];
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

    // One clk cycle. ce_pix must be sampled *before* the edge, because it is a
    // register that flips on the same edge -- the value visible afterwards is
    // the next one, not the one the design just acted on.
    std::vector<uint8_t> fb(FRAME_W * FRAME_H * 3, 0);
    long  pixels_this_frame = 0;
    int   frames_written    = 0;

    // Returns true when a pixel was emitted this cycle. A pixel spans two clk
    // cycles (14 MHz clock, 7 MHz pixel enable), so px_h/px_v hold their value
    // across both -- end-of-frame must be tested only on the cycle that
    // actually produced a pixel, or it fires twice.
    auto tick = [&](bool capture) -> bool {
        top.clk = 0; top.eval();
        const bool ce = top.ce_pix;
        top.clk = 1; top.eval();

        if (!ce) return false;

        const int h = top.px_h, v = top.px_v;
        if (capture && h < FRAME_W && v < FRAME_H) {
            uint8_t* p = fb.data() + (v * FRAME_W + h) * 3;
            p[0] = expand4(top.vga_r);
            p[1] = expand4(top.vga_g);
            p[2] = expand4(top.vga_b);
            pixels_this_frame++;
        }
        return true;
    };

    auto at_frame_end = [&]() {
        return top.px_h == FRAME_W - 1 && top.px_v == FRAME_H - 1;
    };

    // ---- reset -----------------------------------------------------------
    top.rst = 1; top.cpu_we = 0; top.cpu_addr = 0; top.cpu_din = 0; top.border_colour = 0;
    for (int i = 0; i < 8; i++) tick(false);
    top.rst = 0;

    // ---- load the screen through the CPU-side RAM port --------------------
    for (int i = 0; i < SCREEN_BYTES; i++) {
        top.cpu_addr = i;
        top.cpu_din  = scr[i];
        top.cpu_we   = 1;
        tick(false);
    }
    top.cpu_we = 0;

    // ---- run to the end of the current frame, then start capturing --------
    while (!(tick(false) && at_frame_end())) { }
    pixels_this_frame = 0;

    printf("capturing %d frames of %dx%d into %s/\n", frames, FRAME_W, FRAME_H, outdir.c_str());

    while (frames_written < frames) {
        // Border colour comes from a register, cycling once per frame -- the
        // stage 1 milestone, verified here before any hardware is involved.
        top.border_colour = frames_written & 7;

        if (!tick(true)) continue;
        if (!at_frame_end()) continue;

        char path[512];
        snprintf(path, sizeof(path), "%s/frame%02d.bmp", outdir.c_str(), frames_written);
        if (!write_bmp(path, fb.data(), FRAME_W, FRAME_H)) {
            fprintf(stderr, "cannot write %s (does %s/ exist?)\n", path, outdir.c_str());
            return 1;
        }
        printf("  %s  border=%d  pixels=%ld\n", path, frames_written & 7, pixels_this_frame);

        if (pixels_this_frame != (long)FRAME_W * FRAME_H) {
            fprintf(stderr, "  WARNING: expected %d pixels per frame, got %ld\n",
                    FRAME_W * FRAME_H, pixels_this_frame);
        }
        pixels_this_frame = 0;
        frames_written++;
    }

    top.final();
    printf("done\n");
    return 0;
}
