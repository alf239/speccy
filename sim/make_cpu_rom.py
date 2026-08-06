#!/usr/bin/env python3
"""Generate the CPU smoke-test ROM.

Hand-assembled Z80. The program touches each thing the bus testbench already
verified, but this time with a real CPU driving the bus:

  - IM 1, stack at the top of RAM
  - border set to red via OUT (0xFE)
  - one bitmap byte and one attribute byte written to the screen
  - a marker byte written to upper RAM
  - then EI + HALT in a loop, with the IM 1 handler at 0x0038 counting
    interrupts into 0x9000

The testbench watches the counter through a debug tap: one increment per
frame proves the whole interrupt path -- INT asserted at the right line,
acknowledge cycle not answered by the ULA, IM 1 vectoring, EI/RET.
"""

ROM_SIZE = 16384

rom = bytearray(ROM_SIZE)

program = bytes([
    # 0x0000  reset entry
    0xF3,                   # di
    0xED, 0x56,             # im 1
    0x31, 0x00, 0xFF,       # ld sp, 0xFF00
    0x3E, 0x02,             # ld a, 2
    0xD3, 0xFE,             # out (0xFE), a      ; border red
    0x21, 0x00, 0x40,       # ld hl, 0x4000
    0x36, 0xAA,             # ld (hl), 0xAA      ; bitmap byte
    0x3E, 0x55,             # ld a, 0x55
    0x32, 0x00, 0x58,       # ld (0x5800), a     ; attribute byte
    0x3E, 0x5A,             # ld a, 0x5A
    0x32, 0x00, 0x80,       # ld (0x8000), a     ; upper RAM marker
    0xAF,                   # xor a
    0x32, 0x00, 0x90,       # ld (0x9000), a     ; interrupt counter = 0
    0xC3, 0x80, 0x00,       # jp 0x0080          ; continue past the ISR
])
rom[0:len(program)] = program
assert len(program) <= 0x38, "program overlaps the IM 1 vector"

# 0x0080: program the AY -- tone A only, period 50 (2187.5 Hz), volume 15 --
# then the EI + HALT loop. A real CPU driving OUT (C),A at the 128K ports is
# the piece neither the AY unit test nor the bus testbench covers.
ay_setup = bytes([
    0x01, 0xFD, 0xFF,       # ld bc, 0xFFFD
    0x3E, 0x07,             # ld a, 7
    0xED, 0x79,             # out (c), a         ; select R7
    0x06, 0xBF,             # ld b, 0xBF
    0x3E, 0x3E,             # ld a, 0x3E
    0xED, 0x79,             # out (c), a         ; mixer: tone A only
    0x06, 0xFF,             # ld b, 0xFF
    0x3E, 0x00,             # ld a, 0
    0xED, 0x79,             # out (c), a         ; select R0
    0x06, 0xBF,             # ld b, 0xBF
    0x3E, 0x32,             # ld a, 50
    0xED, 0x79,             # out (c), a         ; period 50
    0x06, 0xFF,             # ld b, 0xFF
    0x3E, 0x08,             # ld a, 8
    0xED, 0x79,             # out (c), a         ; select R8
    0x06, 0xBF,             # ld b, 0xBF
    0x3E, 0x0F,             # ld a, 15
    0xED, 0x79,             # out (c), a         ; full volume
    0xFB,                   # ei
    # halt loop
    0x76,                   # halt
    0x18, 0xFD,             # jr halt
])
rom[0x80:0x80 + len(ay_setup)] = ay_setup

isr = bytes([
    # 0x0038  IM 1 handler
    0xF5,                   # push af
    0x3A, 0x00, 0x90,       # ld a, (0x9000)
    0x3C,                   # inc a
    0x32, 0x00, 0x90,       # ld (0x9000), a
    0xF1,                   # pop af
    0xFB,                   # ei
    0xC9,                   # ret
])
rom[0x38:0x38 + len(isr)] = isr

import sys
out = sys.argv[1] if len(sys.argv) > 1 else "sim/cpu_rom.hex"
with open(out, "w") as f:
    f.write("\n".join(f"{b:02X}" for b in rom) + "\n")
print(f"wrote {out}: {len(program)} bytes of program, {len(isr)} bytes of ISR")
