# Scorpion ZS-256 on the DE10-Lite — fit analysis

*2026-07-27 — **verdict: it fits, but the architecture differs from the 48K plan in
one important way.** Revised later the same day: **deferred to phase 2.** The
near-term build is [phase 1: 48K + divMMC](20260727-phase1-48k-divmmc.md), which
stays entirely in block RAM. This document remains the reference for when 128K/256K
and SDRAM become worth taking on.*

## Headline finding: the RAM will not fit in block RAM

This is the key difference from the [48K analysis](20260726-full-spectrum-de10-lite.md),
where the whole machine lived in M9K and the SDRAM was never touched.

| | Needed | Available |
| --- | --- | --- |
| Scorpion RAM | 256 KB = 2,048 Kb | M9K total: 1,638 Kb (≈180 KB usable data) |
| ROM (believed 4 × 16K pages: 128 BASIC, 48 BASIC, TR-DOS, service monitor) | 64 KB = 512 Kb | — |
| **Total** | **320 KB** | **~200 KB** |

Doesn't fit. And cutting down to 128 KB doesn't rescue it either: 128 KB RAM + 64 KB
ROM = 192 KB against ~200 KB available is 96% occupancy, with nothing left for
scandoubler line buffers, SD FIFOs, or the M9K allocation waste that always shows up
in practice.

**So: main RAM moves to the 64 MB SDRAM.** That's the one genuinely new engineering
task compared with the 48K plan.

### Why this is work, not risk

Bandwidth is not remotely a constraint:

- video fetch: 2 bytes per 8 pixels at 7 MHz pixel clock = **1.75 MB/s**
- CPU at 3.5 MHz, worst case ≈ **1 MB/s** (double it for 7 MHz turbo)
- SDRAM at 100 MHz × 16 bits ≈ **200 MB/s**

Two orders of magnitude of headroom, so a **fixed time-slot arbiter** is sufficient —
no clever scheduling, no reordering, fully deterministic. Latency is a non-issue too:
CAS latency at 100 MHz is ~20–30 ns against a Z80 memory cycle of ~857 ns.

The SDRAM is already routed on the board, so it costs nothing from the GPIO headers.

ROM stays in block RAM, initialised from the configuration file (.mif/.hex) at
config time — no extra logic, no UFM access latency to design around.

## Logic budget

| Block | Estimate (LE) |
| --- | --- |
| Z80 core (T80 / TV80) | ~2,500 |
| Video generator + scandoubler | ~2,500 |
| AY-3-8912 | ~1,500 |
| SDRAM controller + arbiter | ~1,000 |
| SD card (SPI mode) | ~500 |
| Beta Disk / WD1793-class FDC | ~2,000–3,000 |
| Housekeeping soft CPU (if needed — see risks) | ~2,000 |
| Glue: paging, port decode, keyboard, turbo | ~1,500 |
| **Total** | **~13,500–16,500 of 50,000** |

Comfortable, roughly a third of the device. Clocking is unchanged from the 48K
analysis: 50 × 7/25 = 14 MHz exactly from a PLL, and turbo mode is just a clock
enable — the video and AY timing must *not* follow it.

## The three physical gaps (unchanged)

The DE10-Lite has no PS/2, no audio, no SD slot.

1. **Keyboard** — PS/2 on two GPIOs with 3.3V pull-ups (works, technically out of
   spec; MAX 10 is not 5V tolerant), or an FPGA low-speed USB-HID host core needing
   two series resistors on D+/D−.
2. **Audio** — now harder than the 48K case, because AY is 3 channels of 4-bit
   logarithmic volume rather than a 1-bit beeper. Sigma-delta or PWM at a high rate
   into an RC filter and a 3.5 mm jack is adequate.
3. **Storage** — microSD breakout on the GPIO header, SPI mode. Now mandatory rather
   than optional, since TR-DOS wants disk images.

## Real risks, ranked *(revised)*

The disk layer was originally ranked #1 here. **divMMC substantially defuses it** —
see [phase 1](20260727-phase1-48k-divmmc.md). Rather than emulating a WD1793 and
parsing images in HDL, divMMC exposes an SPI port plus paging traps and lets
esxDOS — Z80 software — do the filesystem work. That is how ZX-UNO manages storage on
a small FPGA with no host CPU, while MiSTer needs an ARM for the same job.

Revised ranking:

1. **SDRAM arbitration.** Now the largest genuinely new piece of engineering, since
   phase 1 never touches it. Low risk in the sense that bandwidth headroom is two
   orders of magnitude (see above) and a fixed time-slot arbiter suffices — but it is
   the first thing in the project that can silently corrupt video or CPU reads.
2. **Beta Disk / TR-DOS authenticity.** Only needed if TRD images specifically
   matter; divMMC already solves *loading software*. Decouple these two goals —
   "can I load anything" should never block on "is my FDC cycle-accurate".
   **Mitigation:** implement the FDC at sector-read/write level rather than full
   WD1793 emulation. TR-DOS mostly does sector-level operations; low-level access and
   copy protection won't work, which is fine.
3. **Documentation fidelity.** Scorpion schematics circulate in the community rather
   than being formally published. The paging port bits noted here are from memory and
   must be verified before implementation.
4. **Scope creep via SMUC.** IDE and RTC are out of scope. Note and forget.

## Toolchain — settled

Quartus Prime has no macOS build (Windows/Linux x86-64 only), and the Mac here is
arm64. Resolved: an existing 2014-era x86 Windows box already runs Quartus and has
already been used this way over remote desktop. No further investment needed.

**But most work should not happen there.** The whole simulation stack is native
Apple Silicon — Verilator, Icarus, GTKWave (already installed), cocotb — and this is
overwhelmingly a testbench project. Verilator compiles the design to C++, so the
video output can be wired to SDL2 and the entire machine run in a window on the Mac
at near real time, loading a ROM, before the board is touched. Hardware then debugs
*pins*, not logic.

```bash
brew install verilator icarus-verilog
```

## Staging plan *(revised — this is now phase 2)*

Phase 1 (stages 0–5, see [that doc](20260727-phase1-48k-divmmc.md)) delivers a
working 48K machine with divMMC storage, entirely in block RAM. It ends with
Saboteur II running on a monitor. Everything below picks up from there:

6. Move RAM from block RAM to SDRAM — no functional change, purely the memory
   subsystem swap, so it can be verified against a known-good machine
7. Extend to 128K paging (#7FFD)
8. AY sound — **build the volume/envelope DAC as two selectable curves (AY-3-8912
   vs YM2149F) with a board switch on the select line.** The chips differ almost
   only there (YM has 32-step envelope resolution vs 16), so the cost is two
   small tables and a mux — and the switch works live, mid-tune. Proposed
   allocation: SW[8], next to SW[9] = sync polarity. Keep a switch map in the
   board top's header comment as these accumulate. The AY's port A register
   must exist and read back (the 128 hangs its keypad/RS-232 off it; software
   probes it) even though it will be wired to nothing.
9. Scorpion extensions: #1FFD paging, 256K, 7 MHz turbo, service ROM
10. Beta Disk / TR-DOS, if TRD images specifically matter

Stage 6 is deliberately isolated: swapping the memory subsystem underneath a machine
that already works is far easier to debug than building both at once. Stage 7 is
where the block RAM ceiling actually bites, which is the real justification for the
SDRAM work — not the Scorpion features themselves.

**Nostalgia note:** the Scorpion's ROM-resident debugger is a genuine motivation for
getting this far (it was decent preparation for SoftICE later), but it is a stage 9
reward, not a requirement.

## Open decisions

- Keyboard route: PS/2 vs USB-HID core. Affects the parts order — decide before stage 1.
- Survey existing cores (ZX-UNO, MiST/MiSTer Spectrum family) before writing from
  scratch. Not yet done.
- Obtain and verify Scorpion schematics; confirm ROM layout and paging port bits.
