# The real 48K: diagnosing the ULA, and replacing it

*2026-07-27 — outcome: **parked**. Kept for reference in case the machine is
revisited. See [target choice](20260727-clone-lineage-and-target-choice.md) for
where the effort went instead.*

## Symptom

Real ZX Spectrum 48K. Memory and roughly half the surrounding logic already
replaced. Still "works a bit weirdly". Current hypothesis from elsewhere: dead ULA.

## Why the ULA hypothesis is weak

A genuinely dead ULA is not subtle — no video, no 3.5 MHz CPU clock, no interrupts,
machine completely inert. Partial weirdness points elsewhere, and the recently
replaced RAM is the obvious suspect.

Specifically: if the lower 16K 4116s were replaced with an SRAM adapter, be
suspicious of it. The ULA drives that bank itself with multiplexed row/column
addressing and its own RAS/CAS timing, interleaving video fetches with CPU
accesses. The adapter has to de-multiplex that correctly at full speed. Marginal
adapters produce exactly "mostly works, occasional screen garbage, random
crashes" — routinely misdiagnosed as a dying ULA.

**Diagnostic tell:** the upper 32K does not go through the ULA at all. A fault
confined to addresses below 0x8000 implicates the lower bank and its adapter, not
the ULA.

### Second-order caution: calibration

Both reference machines previously owned were Soviet clones with **no memory
contention** and non-Ferranti timing. A correctly functioning real 48K genuinely
behaves differently — anything touching the bottom 16K runs slower, and software
written on the Russian scene for Pentagon-style timing misbehaves in ways that are
*authentic*. Some of "weirdly" may be "correctly, for the first time".

## Cheap tests, in order

1. **Logic analyser (~£8 USB unit) on the ULA pins.** Look for:
   - clean 3.5 MHz on the CPU clock output. Note that 48K contention works by
     *stretching this clock*, not by asserting /WAIT — a ragged or stuck clock is a
     genuine ULA smoking gun
   - /INT pulsing at 50 Hz
   - RAS/CAS cycling
   - sync present on video
2. **Diagnostic ROM.** Brendan Alford's Spectrum Diagnostics is the usual choice and
   has audio-only result modes for when video is dead.

Twenty minutes of work that either confirms or kills the hypothesis. Do this before
building anything.

## Rejected: DE10-Lite as a socket-level ULA emulator

Idea was to drive the ULA socket from the DE10-Lite over ribbon cable to test the
theory. Four problems stack up:

- **Level shifting.** MAX 10 I/O is 3.3V and not 5V tolerant; the Spectrum bus is 5V
  TTL. Roughly 46–50 signals need shifting, and the data bus is bidirectional, so
  that's direction-controlled transceivers — six or seven 74LVC245s.
- **I/O count.** 36 GPIO + ~16 Arduino-header pins. Every pin used, nothing spare.
- **Cable length.** DRAM RAS/CAS timing and a 14 MHz-derived clock down 30 cm of
  ribbon. When it misbehaves you cannot tell a wrong implementation from bad wiring
  — fatal ambiguity for a diagnostic tool.
- **Analog composite.** The ULA generates PAL composite directly via an external
  transistor buffer. An FPGA pin cannot; you'd tap RGB + sync separately.

The adapter board needed to make this work is itself a multi-week PCB project — at
which point you have built the replacement, only worse.

### ULA signal inventory (approximate)

8 data (bidirectional) · ~11 address inputs · 7–8 multiplexed DRAM address out ·
RAS/CAS/WE · 4 CPU control · 5 keyboard · video · CPU clock out · /INT · ROMCS ·
speaker/MIC/EAR · 14 MHz in. **≈ 46–50 signals.**

## A real replacement: a board, not a chip

No modern programmable part exists in 40-pin DIP that is both 5V-tolerant and large
enough; that category died around 2005. Commercial replacements are all small PCBs
with machined DIP pins underneath and a modern part on top. Two silicon options:

| Part | Pros | Cons |
| --- | --- | --- |
| Microchip ATF1508AS (PLCC84, 128 macrocells) | genuinely 5V, still in production, **no level shifting at all** | 128 macrocells is *tight* for a ULA; toolchain is a museum piece (Quartus II 13.0sp1 + Atmel fitter + POF2JED). The 64-macrocell ATF1504 will not fit it. |
| Lattice MachXO2 (e.g. LCMXO2-1200 TQFP100, ~£6) | far more room, modern free toolchain (Diamond) | 3.3V, not 5V tolerant |

Mitigation for the MachXO2: 3.3V CMOS outputs drive 5V TTL inputs fine, so only the
*inbound* signals plus the bidirectional data bus need shifting — a much smaller
problem than all 50.

**Design reference:** Chris Smith, *The ZX Spectrum ULA: How to Design a
Microcomputer*. Gate-by-gate reverse engineering; the reason accurate
reimplementations are possible at all. Read before writing any HDL.

## The catch on "drop-in"

Composite PAL colour from a digital device means synthesising the subcarrier with
correct phase, and the Spectrum's own colour generation is a slightly off-spec trick
to begin with. Most replacement designs sidestep this by outputting RGB and
expecting the RGB pins on the DIN socket or a video mod.

**Decide this early** — if it must work through the RF modulator exactly as before,
that is the single hardest requirement in the project and it shapes everything else.
