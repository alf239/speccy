#!/usr/bin/env python3
"""Convert a 48K .z80 snapshot into memory-init files for the FPGA.

Usage: snap2hex.py game.z80 OUTDIR

Writes into OUTDIR:
  snap_vram.hex   16 KB  0x4000-0x7FFF (screen + low RAM)
  snap_ram.hex    32 KB  0x8000-0xFFFF
  snap_stub.hex   256 B  boot overlay: generated Z80 code that restores every
                         register from the snapshot header and jumps in

The stub is mapped over 0x0000-0x00FF by the machine's snapshot overlay, which
disarms itself on the first opcode fetch at or above 0x0100 -- i.e. the JP at
the end of the stub. From that instant the real ROM is back and the game is
running, registers and all.

Supports .z80 versions 1, 2 and 3, compressed or not, 48K machines only.
"""

import sys
import os


def decompress(data, limit=None):
    out = bytearray()
    i = 0
    while i < len(data) and (limit is None or len(out) < limit):
        if data[i] == 0xED and i + 3 < len(data) and data[i + 1] == 0xED:
            out.extend([data[i + 3]] * data[i + 2])
            i += 4
        else:
            out.append(data[i])
            i += 1
    return bytes(out)


def parse_z80(path):
    d = open(path, "rb").read()
    if len(d) < 30:
        sys.exit(f"{path}: too short to be a .z80 file")

    r = {
        "a": d[0], "f": d[1],
        "bc": d[2] | d[3] << 8, "hl": d[4] | d[5] << 8,
        "pc": d[6] | d[7] << 8, "sp": d[8] | d[9] << 8,
        "i": d[10],
        "de": d[13] | d[14] << 8,
        "bc_": d[15] | d[16] << 8, "de_": d[17] | d[18] << 8,
        "hl_": d[19] | d[20] << 8,
        "a_": d[21], "f_": d[22],
        "iy": d[23] | d[24] << 8, "ix": d[25] | d[26] << 8,
        "iff1": d[27] != 0,
        "im": d[29] & 0x03,
    }
    flags = 0x01 if d[12] == 0xFF else d[12]
    r["r"] = (d[11] & 0x7F) | ((flags & 1) << 7)
    r["border"] = (flags >> 1) & 0x07

    mem = bytearray(48 * 1024)          # 0x4000..0xFFFF

    if r["pc"] != 0:                     # ---- version 1 ----
        body = d[30:]
        raw = decompress(body, 48 * 1024) if (flags & 0x20) else body
        if len(raw) < 48 * 1024:
            sys.exit(f"{path}: v1 image short ({len(raw)} bytes)")
        mem[:] = raw[:48 * 1024]
        version = 1
    else:                                # ---- version 2/3 ----
        extra = d[30] | d[31] << 8
        r["pc"] = d[32] | d[33] << 8
        hw = d[34]
        version = 2 if extra == 23 else 3
        ok48 = {2: (0, 1), 3: (0, 1, 3)}[version]
        if hw not in ok48:
            sys.exit(f"{path}: hardware mode {hw} is not a 48K machine "
                     f"(128K snapshots need the phase-2 machine)")
        page_base = {4: 0x8000, 5: 0xC000, 8: 0x4000}
        i = 32 + extra
        while i + 3 <= len(d):
            ln = d[i] | d[i + 1] << 8
            page = d[i + 2]
            i += 3
            if ln == 0xFFFF:
                blk, i = d[i:i + 16384], i + 16384
            else:
                blk, i = decompress(d[i:i + ln], 16384), i + ln
            if page in page_base:
                off = page_base[page] - 0x4000
                mem[off:off + len(blk)] = blk[:16384]

    if r["pc"] < 0x0100:
        print(f"WARNING: PC=0x{r['pc']:04X} is inside the overlay's shadow "
              f"(0x0000-0x00FF) -- the stub occupies that region at boot, so "
              f"this snapshot cannot resume correctly.")
    elif r["pc"] < 0x4000:
        print(f"note: PC=0x{r['pc']:04X} resumes inside a ROM routine "
              f"(typically PAUSE or a keyboard wait -- normal for menu "
              f"snapshots). The overlay only shadows 0x0000-0x00FF, so this "
              f"is fine.")

    return r, bytes(mem), version


def build_stub(r):
    """Z80 code restoring the full register set, ending in JP pc."""
    s = bytearray()
    lo, hi = lambda v: v & 0xFF, lambda v: (v >> 8) & 0xFF

    s += b"\xF3"                                        # di
    s += {0: b"\xED\x46", 1: b"\xED\x56", 2: b"\xED\x5E"}[r["im"]]
    s += bytes([0x3E, r["i"], 0xED, 0x47])              # ld a,I : ld i,a
    s += bytes([0x3E, r["r"], 0xED, 0x4F])              # ld a,R : ld r,a
    s += bytes([0x31, lo(r["sp"]), hi(r["sp"])])        # ld sp,SP

    # Alternates. AF' via push/pop through BC (uses the two bytes below the
    # snapshot's own stack, which are scratch by convention).
    s += bytes([0x01, r["f_"], r["a_"]])                # ld bc,AF'
    s += b"\xC5\xF1"                                    # push bc : pop af
    s += b"\x08"                                        # ex af,af'
    s += bytes([0x01, lo(r["bc_"]), hi(r["bc_"])])
    s += bytes([0x11, lo(r["de_"]), hi(r["de_"])])
    s += bytes([0x21, lo(r["hl_"]), hi(r["hl_"])])
    s += b"\xD9"                                        # exx

    # Border while A is still free.
    s += bytes([0x3E, r["border"], 0xD3, 0xFE])         # ld a,n : out (fe),a

    # Main AF via the same trick, then the rest of the main set.
    s += bytes([0x01, r["f"], r["a"]])                  # ld bc,AF
    s += b"\xC5\xF1"                                    # push bc : pop af
    s += bytes([0x01, lo(r["bc"]), hi(r["bc"])])
    s += bytes([0x11, lo(r["de"]), hi(r["de"])])
    s += bytes([0x21, lo(r["hl"]), hi(r["hl"])])
    s += bytes([0xDD, 0x21, lo(r["ix"]), hi(r["ix"])])
    s += bytes([0xFD, 0x21, lo(r["iy"]), hi(r["iy"])])

    if r["iff1"]:
        s += b"\xFB"                                    # ei
    s += bytes([0xC3, lo(r["pc"]), hi(r["pc"])])        # jp PC -- disarms

    assert len(s) <= 256, "stub outgrew its page"
    return s.ljust(256, b"\x00")


def write_hex(path, data):
    with open(path, "w") as f:
        f.write("\n".join(f"{b:02X}" for b in data) + "\n")


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    snap, outdir = sys.argv[1], sys.argv[2]
    os.makedirs(outdir, exist_ok=True)

    r, mem, version = parse_z80(snap)
    write_hex(os.path.join(outdir, "snap_vram.hex"), mem[:16384])
    write_hex(os.path.join(outdir, "snap_ram.hex"),  mem[16384:])
    write_hex(os.path.join(outdir, "snap_stub.hex"), build_stub(r))

    print(f"{snap}: v{version}, PC=0x{r['pc']:04X} SP=0x{r['sp']:04X} "
          f"IM{r['im']} {'EI' if r['iff1'] else 'DI'} border={r['border']}")
    print(f"wrote snap_vram.hex, snap_ram.hex, snap_stub.hex to {outdir}/")


if __name__ == "__main__":
    main()
