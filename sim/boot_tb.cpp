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
//
// --all dumps every frame (boot animation); default dumps only the last.
// --expect-smoke asserts on the smoke-test ROM's known output, making this
// runnable as a regression test without any copyrighted ROM present.
// ---------------------------------------------------------------------------

#include <verilated.h>
#include "Vboot_tb_top.h"

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

int main(int argc, char** argv) {
    int         frames = 8;
    bool        all = false, expect_smoke = false;
    std::string outfile = "out/boot.bmp";

    for (int i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--out")    && i + 1 < argc) outfile = argv[++i];
        else if (!strcmp(argv[i], "--all"))                    all = true;
        else if (!strcmp(argv[i], "--expect-smoke"))           expect_smoke = true;
    }

    VerilatedContext ctx;
    ctx.commandArgs(argc, argv);
    Vboot_tb_top top(&ctx);

    top.rst = 1; top.key_matrix = 0; top.joy_state = 0; top.ear_in = 1;
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

    while (frame_no < frames) {
        top.clk = 0; top.eval();
        top.clk = 1; top.eval();

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

    top.final();
    printf(failures ? "FAILED (%d)\n" : "ok\n", failures);
    return failures ? 1 : 0;
}
