#!/usr/bin/env python3
"""Build a tiny synthetic v1 .z80 snapshot for regression testing.

Program at 0x8000:  ld (0x4000),hl ; ld a,2 ; out (0xFE),a ; jr $
Registers: HL=0xAAAA, PC=0x8000, SP=0xFF00, IM 1, DI, border 5.
Attribute at 0x5800 = 0x47 (bright white ink) so the written bitmap byte is
visible. The stub sets border 5, the program sets border 2 -- seeing 2 proves
the snapshot's own code executed, not just the stub.
"""
import sys

mem = bytearray(48 * 1024)                    # 0x4000..0xFFFF
mem[0x8000 - 0x4000:0x8000 - 0x4000 + 8] = bytes(
    [0x22, 0x00, 0x40,   # ld (0x4000),hl
     0x3E, 0x02,         # ld a,2
     0xD3, 0xFE,         # out (0xFE),a
     0x18, 0xFE])        # jr $
mem[0x5800 - 0x4000] = 0x47                   # bright white ink, black paper
mem[0x5801 - 0x4000] = 0x47

hdr = bytearray(30)
hdr[0] = 0x00                                 # A
hdr[1] = 0x00                                 # F
hdr[4], hdr[5] = 0xAA, 0xAA                   # HL = 0xAAAA
hdr[6], hdr[7] = 0x00, 0x80                   # PC = 0x8000  (v1 marker)
hdr[8], hdr[9] = 0x00, 0xFF                   # SP = 0xFF00
hdr[12] = 5 << 1                              # border 5, uncompressed
hdr[27] = 0                                   # DI
hdr[29] = 1                                   # IM 1

out = sys.argv[1] if len(sys.argv) > 1 else "out/test.z80"
open(out, "wb").write(bytes(hdr) + bytes(mem))
print(f"wrote {out}")
