// ---------------------------------------------------------------------------
// Behavioural SDHC card, SPI mode. Enough of the protocol for esxDOS:
//
//   CMD0   GO_IDLE           -> R1 idle
//   CMD8   SEND_IF_COND      -> R1 idle + R7 echo (2.7-3.6V, check pattern)
//   CMD55  APP_CMD           -> R1
//   ACMD41 SD_SEND_OP_COND   -> R1 idle for the first few polls, then ready
//   CMD58  READ_OCR          -> R1 + OCR with CCS=1 (SDHC: block addressing)
//   CMD16  SET_BLOCKLEN      -> R1 (accepted, ignored -- SDHC is 512 fixed)
//   CMD17  READ_SINGLE_BLOCK -> R1, gap, 0xFE token, 512 bytes, CRC16 dummy
//   CMD24  WRITE_BLOCK       -> R1, data token+512+CRC in, data-accepted, busy
//
// Wire-level: sample MOSI on SCK rising, present next MISO bit on falling,
// exactly the card's half of SPI mode 0. Backed by a disk image in memory.
// ---------------------------------------------------------------------------
#pragma once
#include <cstdint>
#include <vector>
#include <cstdio>

struct SdModel {
    bool verbose = false;
    std::vector<uint8_t> disk;
    bool     idle = true;            // pre-ACMD41
    int      acmd41_polls = 0;
    bool     app_cmd = false;

    // wire state -- ONE bit counter frames both directions, like a real card:
    // the output byte reloads at the 8-bit input boundary, so response bytes
    // stay aligned with the master's exchanges.
    bool     last_sck = false;
    uint8_t  in_byte = 0;
    int      bit_idx = 0;            // 0..7 within the current byte
    uint8_t  out_byte = 0xFF;
    bool     miso_level = true;
    std::vector<uint8_t> out_q;      // bytes queued for MISO
    size_t   out_pos = 0;

    std::vector<uint8_t> cmd;        // command bytes collected
    // multi-block read (CMD18): stream blocks until CMD12
    bool     multi_read = false;
    uint32_t multi_lba = 0;
    // write path
    bool     expecting_data = false;
    std::vector<uint8_t> wr_buf;
    uint32_t wr_lba = 0;

    explicit SdModel(size_t blocks = 4096) : disk(blocks * 512, 0) {}

    void queue(uint8_t b) { out_q.push_back(b); }

    void queue_block(uint32_t lba) {
        queue(0xFF);                              // access gap
        queue(0xFE);                              // data token
        const uint64_t off = (uint64_t)lba * 512;
        for (int i = 0; i < 512; i++)
            queue(off + i < disk.size() ? disk[off + i] : 0xFF);
        queue(0xAA); queue(0x55);                 // CRC16, unchecked
    }
    void queue_r1(uint8_t r1) { queue(0xFF); queue(r1); }   // Ncr then R1

    void handle_cmd() {
        const uint8_t c = cmd[0] & 0x3F;
        const uint32_t arg = (uint32_t)cmd[1] << 24 | cmd[2] << 16 | cmd[3] << 8 | cmd[4];
        if (verbose) fprintf(stderr, "SD: %sCMD%d arg=%08X\n",
                             app_cmd ? "A" : "", c, arg);
        const bool was_app = app_cmd;
        app_cmd = false;

        if (was_app && c == 41) {                    // ACMD41
            if (++acmd41_polls >= 3) idle = false;
            queue_r1(idle ? 0x01 : 0x00);
            return;
        }
        switch (c) {
            case 0:  idle = true; acmd41_polls = 0; queue_r1(0x01); break;
            case 8:                                   // R7: echo voltage+pattern
                queue_r1(0x01);
                queue(0x00); queue(0x00);
                queue((arg >> 8) & 0xFF); queue(arg & 0xFF);
                break;
            case 55: app_cmd = true; queue_r1(idle ? 0x01 : 0x00); break;
            case 58:                                  // OCR: powered, CCS=1
                queue_r1(idle ? 0x01 : 0x00);
                queue(0xC0); queue(0xFF); queue(0x80); queue(0x00);
                break;
            case 16: queue_r1(0x00); break;
            case 9: {                                 // SEND_CSD (v2, SDHC)
                queue_r1(0x00);
                queue(0xFF); queue(0xFE);
                uint32_t csize = (uint32_t)(disk.size() / (512 * 1024)) - 1;
                uint8_t csd[16] = {0x40,0x0E,0x00,0x32,0x5B,0x59,0x00,0x00,
                                   (uint8_t)(csize >> 16), (uint8_t)(csize >> 8),
                                   (uint8_t)csize, 0x7F,0x80,0x0A,0x40,0x01};
                for (int i = 0; i < 16; i++) queue(csd[i]);
                queue(0x00); queue(0x00);
                break;
            }
            case 10: {                                // SEND_CID
                queue_r1(0x00);
                queue(0xFF); queue(0xFE);
                const char* cid = "\x03SPECCY-SIM-C1\x01";
                for (int i = 0; i < 16; i++) queue((uint8_t)cid[i]);
                queue(0x00); queue(0x00);
                break;
            }
            case 12:                                  // STOP_TRANSMISSION
                multi_read = false;
                out_q.clear(); out_pos = 0;           // abandon the stream
                queue(0xFF);                          // stuff byte
                queue_r1(0x00);
                break;
            case 17:                                  // read single block
                queue_r1(0x00);
                queue_block(arg);
                break;
            case 18:                                  // read multiple blocks
                queue_r1(0x00);
                multi_read = true;
                multi_lba = arg;
                queue_block(multi_lba++);
                break;
            case 24:                                  // write block
                queue_r1(0x00);
                expecting_data = true;
                wr_buf.clear();
                wr_lba = arg;
                break;
            default: queue_r1(idle ? 0x05 : 0x04);    // illegal command
        }
    }

    long dbg_count = 0;
    void byte_in(uint8_t b) {
        if (verbose && dbg_count < 120)
            fprintf(stderr, "%s%02X", (dbg_count++ % 24) ? " " : "\nRX: ", b);
        if (expecting_data) {
            if (wr_buf.empty() && b == 0xFF) return;  // pre-token filler
            wr_buf.push_back(b);
            // token + 512 + 2 CRC
            if (wr_buf.size() >= 515) {
                if (wr_buf[0] == 0xFE) {
                    const uint64_t off = (uint64_t)wr_lba * 512;
                    for (int i = 0; i < 512 && off + i < disk.size(); i++)
                        disk[off + i] = wr_buf[1 + i];
                    queue(0x05);                      // data accepted
                    queue(0x00); queue(0xFF);         // brief busy, then free
                } else {
                    queue(0x0D);                      // rejected
                }
                expecting_data = false;
            }
            return;
        }
        if (cmd.empty()) {
            if ((b & 0xC0) == 0x40) cmd.push_back(b); // start+transmission bits
            return;                                    // else: idle 0xFF filler
        }
        cmd.push_back(b);
        if (cmd.size() == 6) { handle_cmd(); cmd.clear(); }
    }

    uint8_t next_out() {
        if (out_pos >= out_q.size() && multi_read)
            queue_block(multi_lba++);                 // CMD18 keeps streaming
        uint8_t b = (out_pos < out_q.size()) ? out_q[out_pos++] : 0xFF;
        if (out_pos >= out_q.size() && !multi_read) { out_q.clear(); out_pos = 0; }
        return b;
    }

    // Advance one system clock; returns MISO level.
    bool step(bool cs_n, bool sck, bool mosi) {
        if (cs_n) {                                    // deselected: reset framing
            last_sck = sck; bit_idx = 0; miso_level = true;
            return true;
        }
        if (sck && !last_sck) {                        // rising: sample MOSI
            in_byte = (uint8_t)((in_byte << 1) | (mosi ? 1 : 0));
            if (++bit_idx == 8) {
                byte_in(in_byte);                      // may queue a response
                bit_idx = 0;
                out_byte = next_out();                 // aligned to the boundary
            }
        } else if (!sck && last_sck) {                 // falling: present the bit
            miso_level = ((out_byte >> (7 - bit_idx)) & 1) != 0;
        }
        last_sck = sck;
        return miso_level;
    }
};
