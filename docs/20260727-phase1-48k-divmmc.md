# Phase 1: a 48K Speccy on the DE10-Lite, with divMMC

*2026-07-27 — **the plan to actually build.** Supersedes the Scorpion build as the
near-term target; see [Scorpion fit](20260727-scorpion-on-de10-lite.md) for phase 2.*

## Goal

Run **Saboteur II** on a real monitor, on real hardware, on a machine built from
scratch. The driving question is "can I actually do this?", not "can I own a
Scorpion" — which means the right target is the *smallest* machine that satisfies it.

Saboteur II is a 48K game. It doesn't need 128K, doesn't need AY, doesn't need
TR-DOS, and — importantly — doesn't lean on contention, floating bus or multicolour
tricks. A non-contended 48K runs it.

## Target machine

**A 48K Spectrum with Pentagon-style timing (no contention), divMMC storage, VGA
out.** Deliberately not a 48K *Ferranti* machine: contention is the months-long
fidelity grind and it buys nothing for this goal.

## The key finding: it all fits in block RAM

This is what makes phase 1 worth separating out. The DE10-Lite has 1,638 Kb of M9K,
which after the 9-bit-per-word organisation is **≈180 KB of usable data**.

| Block | Size |
| --- | --- |
| Spectrum RAM | 48 KB |
| Spectrum ROM | 16 KB |
| esxDOS ROM (divMMC) | 8 KB |
| divMMC RAM | 32 KB |
| Scandoubler line buffers | ~2 KB |
| **Total** | **~106 KB of ~180 KB (59%)** |

**No SDRAM. No memory controller. No arbiter.** The video generator reads screen
memory in the same cycle as the CPU, exactly like real hardware, because both sit in
dual-port block RAM.

The dividing line is sharp and worth knowing: a **128K** machine needs 128 KB RAM +
32 KB ROM + divMMC on top, which lands at ~1,600 Kb against 1,638 Kb — unworkable in
practice. So "stay in block RAM" and "stay at 48K" are the same decision. Phase 2
(128K, Scorpion) is exactly the point where SDRAM becomes unavoidable.

## Logic budget

| Block | Estimate (LE) |
| --- | --- |
| Z80 core (T80 / TV80) | ~2,500 |
| Video generator + scandoubler | ~2,500 |
| divMMC (SPI master + automapper) | ~500 |
| PS/2 keyboard | ~200 |
| Beeper (1-bit — no DAC needed) | ~50 |
| Glue: port decode, Kempston, reset | ~500 |
| **Total** | **~6,250 of 50,000 (12.5%)** |

The entire machine occupies about an eighth of the chip.

Clocking as before: 50 × 7/25 = **14 MHz exactly** from a PLL, dividing to 7 MHz
pixel and 3.5 MHz CPU.

## Storage: divMMC instead of an FDC

The single biggest change from the Scorpion plan. Rather than emulating a WD1793 and
parsing disk images in HDL, divMMC hands the problem to the Z80:

- **an SPI master** talking to the SD card in SPI mode — a shift register, ~200–300 LE
- **an automapper**: paging logic watching the address bus during M1 cycles, swapping
  the esxDOS ROM in at known entry points (believed 0x0000, 0x0008, 0x0038, 0x0066,
  0x04C6, 0x0562) and out again in the 0x1FF8–0x1FFF range — *verify against the
  divIDE spec before implementing*
- **a control port** (0xE3 for divIDE/divMMC; divMMC adds SPI data/select ports around
  0xE7/0xEB — also worth verifying)
- 8 KB ROM + 32 KB RAM, both in block RAM

All the hard work — FAT parsing, directory browsing, file loading, `.TAP` playback —
is done by **esxDOS, which is Z80 software**. That is the whole trick, and it's why
ZX-UNO can do storage on a small FPGA with no host CPU while MiSTer needs an ARM.

Practically: `saboteur2.tap` goes on the SD card, esxDOS hooks the ROM loader and
feeds it. No tape, no FDC, no waiting.

## Physical gaps — now only three small ones

1. **Keyboard.** PS/2 on two GPIO pins with 3.3V pull-ups. ~200 LE for the core.
   **Buy a genuine PS/2 keyboard** — passive USB→PS/2 adapters only work with
   dual-protocol keyboards, which are long extinct. MAX 10 is not 5V tolerant; check
   levels. (USB-HID host core remains an option later; not worth it for phase 1.)
2. **Audio.** Saboteur II is a 48K game, so beeper only: one pin, a resistor and a
   cap into a 3.5 mm jack. AY is a phase 2 concern.
3. **Storage.** A ~£2 microSD breakout on the GPIO header.

### Free hardware already on the board

- **10 slide switches + 2 buttons** — enough for a Kempston joystick (up/down/left/
  right/fire) before any keyboard exists. Awkward to actually play Saboteur II with,
  but a legitimate "it works" milestone.
- **6× 7-segment displays** — genuinely useful for debug: display the PC, the current
  port value, or an SPI state machine's state, without needing SignalTap.

## Staging

0. **Verilator + SDL harness on the Mac.** Whole machine in a window, native arm64,
   full speed. Most work happens here.
1. VGA output + border colour from a register, on hardware.
2. Video generator reading a static screen from block RAM.
3. T80 + 48K ROM booting BASIC. **Keyboard needed here** — first hard blocker.
4. Beeper.
5. divMMC + SD card → load Saboteur II.

Goal achieved at stage 5. Everything after that is phase 2.

## Risks, re-ranked for phase 1

The disk layer was ranked #1 in the Scorpion analysis. divMMC largely dissolves it.
What remains:

1. **Keyboard.** The first thing that can actually stop progress — you can't reach
   stage 3 without it. Order a PS/2 keyboard now.
2. **Getting a stable picture on a real monitor.** 50 Hz over VGA is accepted by most
   LCDs but not all; the scandoubler and sync generation are where "it works in
   simulation" meets a display that disagrees. Have a fallback monitor.
3. **divMMC automapper correctness.** Small but fiddly — it watches M1 cycles and
   mispaging produces confusing crashes. Well documented, so it's care rather than
   invention.

Explicitly **not** risks any more: SDRAM timing, memory contention, floating bus,
WD1793 emulation, AY, TR-DOS, extended paging. All deferred or deleted.

## Out of scope for phase 1

128K paging · SDRAM · AY sound · Beta Disk / TR-DOS · Scorpion service ROM · turbo
mode · contention accuracy · composite output

## Toolchain

Simulation native on the Mac (Verilator, Icarus, GTKWave); synthesis on the existing
x86 Windows box over remote desktop. See the toolchain section of the
[Scorpion doc](20260727-scorpion-on-de10-lite.md) for detail.

```bash
brew install verilator icarus-verilog
```

## Reference

- **ZX-UNO** — the closest match to this situation: small FPGA, no host CPU, divMMC
  in HDL. Spartan-6/VHDL, so porting means translating clock primitives, but the
  divMMC implementation is the thing to read.
- **MiSTer Spectrum core** — good video generator and clean multi-machine structure,
  but assumes an ARM host (`hps_io`) for anything touching files.
