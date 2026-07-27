# ZX Spectrum 48K on the DE10-Lite — feasibility notes

*2026-07-26 — exploratory. **Superseded 2026-07-27:** target changed to the Scorpion
ZS-256, see [target choice](20260727-clone-lineage-and-target-choice.md) and
[Scorpion fit](20260727-scorpion-on-de10-lite.md). Retained because the clocking,
video and physical-gap analysis all still apply; the block-RAM conclusion does not.*

## Verdict

Yes, comfortably. The FPGA is overkill for a 48K; every interesting constraint is
I/O and timing, not logic capacity.

## Board

Terasic DE10-Lite, Intel MAX 10 `10M50DAF484C7G`:

- 50K logic elements
- 1,638 Kb M9K block RAM
- ~5,888 Kb (≈700 KB) on-die user flash (UFM)
- 64 MB SDRAM (x16)
- VGA out via 4-bit-per-channel resistor DAC
- 2×20 GPIO header + Arduino Uno R3 header (3.3V I/O, **not** 5V tolerant)
- 2× 50 MHz oscillators, 4 PLLs
- On-board USB Blaster
- No audio codec, no PS/2, no SD slot, no HDMI

## Resource budget

| Resource | 48K needs | DE10-Lite has |
| --- | --- | --- |
| Logic | ~8–10K LE (T80 core ~2.5K, ULA ~1.5K, scandoubler + glue) | 50K LE |
| Block RAM | 64 KB = 512 Kb (16K ROM + 48K RAM) | 1,638 Kb M9K |
| Clocks | 14 MHz master → 7 MHz pixel → 3.5 MHz CPU | 50 MHz; 50 × 7/25 = 14 MHz exactly from a PLL |
| Colour out | 3-bit RGB + BRIGHT | VGA 4:4:4 |

Notes:

- **The SDRAM is never touched.** The whole machine fits in block RAM. That means
  no memory controller, no latency games, and the ULA can read video memory in the
  same cycle as the CPU exactly like the real chip. Keep the SDRAM in reserve for a
  possible +2/+3 or divIDE later.
- **VGA is a lucky match.** The 15 Spectrum colours map onto a handful of 4-bit
  levels, and 50 Hz over VGA is accepted by most LCD monitors — so a plain
  scandoubler (15.625 → 31.25 kHz) is the entire video job.

## Missing hardware — three gaps, all cheap

1. **Keyboard.** No PS/2 jack, no USB host.
   - *Option A:* PS/2 keyboard on two GPIO pins with 3.3V pull-ups. Works in
     practice, technically out of spec. MAX 10 I/O is not 5V tolerant — verify
     levels before connecting.
   - *Option B:* an FPGA low-speed USB-HID host core, needing only two series
     resistors on D+/D−. More work up front, works with any modern keyboard.
2. **Audio.** Nothing on board — but the 48K beeper is 1 bit, so a resistor + cap
   into a 3.5 mm jack is a genuinely complete solution.
3. **Storage.** No SD slot.
   - A ~£2 microSD breakout on the GPIO header in SPI mode gives .TAP/.Z80 loading.
   - Or: put the ROM plus a few games in the MAX 10 user flash — zero external
     parts, a good stepping stone for first bring-up.

## The actually hard part: ULA timing

Fit is not the challenge. Cycle-accuracy is the whole project. Booting BASIC takes
a weekend; running demos and later games correctly means reproducing:

- 224 T-states per line, 312 lines, 69,888 T-states per frame (50.08 Hz)
- Memory contention on the 0x4000–0x7FFF bank, in the ULA's 6-of-8 T-state pattern
- Floating bus reads, and the exact position and length of the interrupt

Plenty of cores boot the ROM. Far fewer pass the standard timing test suites.

## Suggested bring-up order

Each stage independently testable, so there's something on screen early rather than
a whole machine to debug at once:

1. VGA output + border colour from a register
2. ULA reading a static screen from block RAM
3. Drop in the T80 CPU core with the ROM
4. Keyboard
5. Beeper
6. Loader (UFM first, then SD)

## Open questions / next steps

- Sketch the ULA timing module and clock/PLL setup, **or**
- Build the stage-one VGA + border test first.
- Decide keyboard route (PS/2 vs USB-HID core) — affects the parts order.
- Existing cores (MiST/MiSTer Spectrum, ZX-UNO) are worth reviewing as reference or
  porting bases before writing from scratch. Not yet evaluated.
