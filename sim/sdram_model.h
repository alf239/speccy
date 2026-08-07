// ---------------------------------------------------------------------------
// Behavioural SDR SDRAM (IS42S16320-flavoured, 16-bit wide), enough protocol
// to keep a controller honest:
//
//   - decodes ACTIVE / READ / WRITE / PRECHARGE / AUTO REFRESH / LOAD MODE
//     from {cs_n, ras_n, cas_n, we_n} each clock
//   - requires the init ritual before any ACTIVE: precharge-all, >=2 auto
//     refreshes, load mode -- violations are counted as protocol errors
//   - tracks one open row per bank; READ/WRITE to a closed bank is an error
//   - CL=2 read pipeline: data appears two clocks after the READ command
//   - DQM lanes honoured on writes (per-byte) -- reads ignore DQM here, the
//     controller drives both lanes low
//   - auto-precharge (A10 at READ/WRITE) closes the row afterwards
//   - refresh interval watchdog: reports the worst gap seen so the test can
//     assert the 7.8 us budget was never blown (in clocks, caller's scale)
//
// Storage is a modest array with address masking -- the Spectrum uses 96 KB
// of a 64 MB part, simulating the full part would be vanity.
// ---------------------------------------------------------------------------
#pragma once
#include <cstdint>
#include <cstdio>
#include <vector>

struct SdramModel {
    static constexpr int  ADDR_BITS = 20;               // 1M words simulated
    static constexpr long ROWS      = 8192;

    std::vector<uint16_t> mem;
    bool     verbose = false;
    long     errors  = 0;

    // init ritual tracking
    bool     saw_precharge_all = false;
    int      init_refreshes    = 0;
    bool     mode_loaded       = false;

    // per-bank open row (-1 = closed)
    long     open_row[4] = {-1, -1, -1, -1};

    // CL=2 pipeline: value scheduled for N cycles ahead
    int      pipe_cnt[3] = {0, 0, 0};
    uint16_t pipe_val[3] = {0, 0, 0};
    bool     driving = false;
    uint16_t dq_drive = 0;

    // auto-precharge bookkeeping for reads (row closes after data)
    // refresh watchdog
    long     clk_count = 0, last_refresh = 0, worst_gap = 0;

    SdramModel() : mem(1u << ADDR_BITS, 0xFFFF) {}

    void err(const char* what) {
        errors++;
        fprintf(stderr, "SDRAM MODEL: %s (clk %ld)\n", what, clk_count);
    }

    uint32_t index(int bank, long row, int col) {
        return ((uint32_t)row * 1024u + (uint32_t)col + (uint32_t)bank * 0x100000u)
               & ((1u << ADDR_BITS) - 1);
    }

    // One controller clock: sample the command bus (as latched by the chip on
    // its own clock, half a period later -- at 14 MHz the distinction never
    // bites, so sampling per controller cycle is faithful enough).
    // Returns the value the chip drives on DQ, valid when driving.
    bool step(bool cs_n, bool ras_n, bool cas_n, bool we_n,
              uint16_t a, int ba, bool ldqm, bool udqm,
              uint16_t dq_from_ctrl, bool ctrl_oe, uint16_t& dq_to_ctrl) {
        clk_count++;

        // advance the read pipeline
        driving = false;
        for (int i = 0; i < 3; i++) {
            if (pipe_cnt[i] > 0 && --pipe_cnt[i] == 0) {
                dq_drive = pipe_val[i];
                driving  = true;
            }
        }
        // hold the data one extra cycle so a latch on the next edge sees it
        if (driving) dq_to_ctrl = dq_drive;

        if (cs_n) return driving;
        const int cmd = ((ras_n ? 4 : 0) | (cas_n ? 2 : 0) | (we_n ? 1 : 0));

        switch (cmd) {
            case 7: break;                                   // NOP
            case 3: {                                        // ACTIVE
                if (!mode_loaded) err("ACTIVE before init complete");
                if (open_row[ba] >= 0) err("ACTIVE on already-open bank");
                open_row[ba] = a & 0x1FFF;
                break;
            }
            case 5: {                                        // READ
                if (open_row[ba] < 0) { err("READ on closed bank"); break; }
                uint16_t v = mem[index(ba, open_row[ba], a & 0x3FF)];
                // CL=2: present two cycles from now
                for (int i = 0; i < 3; i++)
                    if (pipe_cnt[i] == 0) { pipe_cnt[i] = 2; pipe_val[i] = v; break; }
                if (a & 0x400) open_row[ba] = -1;             // auto-precharge
                break;
            }
            case 4: {                                        // WRITE
                if (open_row[ba] < 0) { err("WRITE on closed bank"); break; }
                if (!ctrl_oe) err("WRITE with controller not driving DQ");
                uint32_t ix = index(ba, open_row[ba], a & 0x3FF);
                uint16_t v = mem[ix];
                if (!ldqm) v = (uint16_t)((v & 0xFF00) | (dq_from_ctrl & 0x00FF));
                if (!udqm) v = (uint16_t)((v & 0x00FF) | (dq_from_ctrl & 0xFF00));
                mem[ix] = v;
                if (a & 0x400) open_row[ba] = -1;             // auto-precharge
                break;
            }
            case 2: {                                        // PRECHARGE
                if (a & 0x400) {
                    for (auto& r : open_row) r = -1;
                    saw_precharge_all = true;
                } else open_row[ba] = -1;
                break;
            }
            case 1: {                                        // AUTO REFRESH
                if (!saw_precharge_all) err("REFRESH before PRECHARGE ALL");
                for (auto& r : open_row)
                    if (r >= 0) { err("REFRESH with a bank open"); break; }
                if (!mode_loaded) init_refreshes++;
                long gap = clk_count - last_refresh;
                if (mode_loaded && gap > worst_gap) worst_gap = gap;
                last_refresh = clk_count;
                break;
            }
            case 0: {                                        // LOAD MODE
                if (init_refreshes < 2) err("LOAD MODE before 2 refreshes");
                // CL=2, burst 1 expected: A6:4=010, A2:0=000
                if (((a >> 4) & 7) != 2 || (a & 7) != 0)
                    err("unexpected mode register value");
                mode_loaded = true;
                last_refresh = clk_count;
                break;
            }
            default: err("reserved command"); break;
        }
        if (verbose && cmd != 7)
            fprintf(stderr, "SDRAM: cmd %d ba %d a %04X\n", cmd, ba, a);
        return driving;
    }
};
