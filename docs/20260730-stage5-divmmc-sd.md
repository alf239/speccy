# Stage 5: divMMC + SD — "tape" loading without the tape

*2026-07-30 — design. SD breakout board in hand; goal: .TAP/.Z80 files load
from SD with no recompile, via esxDOS.*

## Capacity audit — does it fit?

| Resource | In use now | Stage 5 adds | Total | Available |
| --- | --- | --- | --- | --- |
| Block RAM | ~113 KB (ROM 16, vram 16, ramhi 32, snapshot shadows 48, stub ¼, line buffers ~1) | esxDOS ROM 8 KB + divMMC RAM 32 KB | **~153 KB** | ~182 KB |
| Logic | ~7K LE | SPI master ~300, automapper ~200 | **~7.5K LE** | 50K |
| Pins | joystick 5, PS/2 2, beeper 1 | SD: CS, SCK, MOSI, MISO = 4 | 12 | 36 |

Verdict: fits with ~29 KB of M9K to spare. The snapshot shadows are the big
optional: if space ever pinches, snapshot mode and divMMC mode could share
banks, but at current numbers both coexist.

## Pin plan — the silkscreen finally tells the truth

The free odd-column pins include header 19, 21, 23 — whose Pi-cobbler labels
are `P10(MOSI)`, `P09(MISO)`, `P11(SCLK)`, because that's where the Pi's SPI
bus lives. For the first time in this project, the labels describe our wiring:

| Signal | GPIO | Header pin | Cobbler hole says |
| --- | --- | --- | --- |
| SD /CS | GPIO[12] | **15** | `P22` |
| SD MOSI | GPIO[16] | **19** | `P10 (MOSI)` ✓ |
| SD MISO | GPIO[18] | **21** | `P09 (MISO)` ✓ |
| SD SCK | GPIO[20] | **23** | `P11 (SCLK)` ✓ |
| SD 3.3 V | — | **29** | `P05` (verified 3.3 V in pre-flight) |
| SD GND | — | any `GND` hole | |

All odd-column, all clear of the cobbler's ground pour.

**Breakout board check before wiring:** two species exist.
- *Plain 3.3 V breakout* (just the socket, maybe pull-ups/caps): power from
  pin 29's 3.3 V, all four signals direct. Ideal.
- *Arduino-style with regulator + level shifter* (has an SOT-23 regulator and
  a buffer chip like 74LVC125): feed its VCC from **5 V** (pin 11 shares fine
  with the keyboard), and check whether MISO comes back at 5 V — if the
  shifter is powered at 5 V, put the standard 2.2 kΩ in series with MISO.
  MOSI/SCK/CS *into* the board are fine at 3.3 V either way.

SD cards themselves are 3.3 V devices; in SPI mode they draw up to ~100 mA
transients — decoupling on the breakout matters, most boards have it.

## Architecture: divMMC, as planned since day one

- **`spi_master.v`** — mode 0, 8-bit transfers. Clock: ≤400 kHz during card
  init (SD spec), then 7 MHz (14 MHz / 2) for data. Exposed to the Z80 as the
  divMMC ports: **0xEB** data (write = start 8-bit exchange, read = last
  received byte), **0xE7** chip select.
- **Automapper** — the divMMC magic. Watches M1 fetches: entry points
  (0x0000, 0x0008, 0x0038, 0x0066, 0x04C6, 0x0562) page the esxDOS ROM into
  0x0000-0x1FFF instantly; fetches in 0x1FF8-0x1FFF page it back out
  (delayed). 0x2000-0x3FFF becomes banked divMMC RAM per port **0xE3**
  (CONMEM/MAPRAM bits + bank select). All the FAT/TAP work is esxDOS — Z80
  software — exactly the ZX-UNO trick that avoids needing a host CPU.
- **Interaction with the snapshot overlay:** SW[1] snapshot mode must gate
  the automapper OFF (a snapshot boot must not trap into esxDOS). Normal
  boot with SW[1] down: reset traps at 0x0000 → esxDOS initialises, then
  hands over to BASIC with its hooks armed.
- **NMI menu:** KEY[1] is free — it becomes the classic divMMC NMI button:
  press for the esxDOS file browser, pick a .TAP, it loads. This is the
  no-keyboard-needed loading path; LOAD "" with hooks works too once the
  AT keyboard lands.

## esxDOS licensing/logistics

esxDOS is freely distributed under its own terms — like the 48K ROM, it is
fetched locally and never committed (`ESXMMC.BIN` 8 KB → `tools/bin2hex.py` →
`quartus/esxdos.hex`; its `SYS/` + `BIN/` folders go on the SD card, FAT16/32).

## Verification plan (the usual religion)

The centrepiece: an **SD card model in C++** for the Verilator harness —
SPI-mode state machine answering CMD0/CMD8/CMD58/ACMD41/CMD17 (+CMD24 for
writes later) against a disk-image file. Then the flagship test is *booting
esxDOS in simulation* against a small FAT image containing a .TAP, driving
the NMI browser with the PS/2 typist, and watching the virtual monitor.
`make test` gains an spitest (unit: SPI master timing) and an esxtest
(integration, needs local esxDOS binaries, skipped in their absence like
the ROM tests).

## Order of work

1. `spi_master.v` + unit testbench (no SD needed — loopback + timing checks)
2. C++ SD model + CMD-level test against the SPI master
3. divMMC ports + automapper + paging in speccy.v (+ lint, + regression that
   snapshot mode still boots with automapper gated)
4. esxDOS boot in simulation, NMI browser on the virtual monitor
5. Hardware: wire per the table, `git pull`, recompile, insert card
6. Retire the per-game recompile ceremony with honours

## Breakout pinout (as identified, plain 3.3 V board)

DI=MOSI -> pin 19, D0=MISO -> pin 21, CLK=SCK -> pin 23, CS -> pin 15,
3v -> pin 29, G -> GND. DT1/DT2 (SD-mode DAT1/2) unconnected in SPI mode;
CD (card-detect switch) optional. Six wires, no resistors -- everything
native 3.3 V.

## Progress log

- 2026-07-30: spi_master.v + behavioural SDHC model (sim/sd_model.h) done and
  green: init conversation, CMD17 read, CMD24 write + readback, both clock
  speeds, mode-0 phase verified. 22 checks in `make sdtest`. Next: divMMC
  ports (0xE3/0xE7/0xEB) + automapper + paging in speccy.v.
- 2026-08-02: step 3 done -- divMMC in speccy.v: automapper (delayed entry/
  exit traps incl. 0x3Dxx), port 0xE3 with sticky MAPRAM + bank3 write
  protection, 0xE7 CS, 0xEB SPI with WAIT-stretched access (the Z80's wait_n,
  unused until now, absorbs the 350 kHz init clock), auto speed-up after 1024
  exchanges, NMI button on KEY[1], SW[0] = divMMC enable (snapshot mode
  outranks it). 24 new machine-level checks in bustest; suite total 131.
  Board builds now ALSO need quartus/esxdos.hex (bin2hex from esxDOS's
  ESXMMC.BIN -- gitignored like the ROMs). Next: step 4, esxDOS boot in
  simulation against the SD model + a FAT image.
- 2026-08-02: step 4 done -- esxDOS 0.8.9 BOOTS IN SIMULATION: banner, card
  detect (our CID string), FAT16 mount (our generated image), SYS modules
  [OK], NMI file browser listing the card. Four bugs found by the boot, none
  by the unit tests -- integration is undefeated:
    1. esxDOS needs >=5 divRAM banks (it inits banks 4..0); divRAM is now
       64K/8 banks, unified with the snapshot shadow (copier sources it; a
       mixed esxDOS-then-snapshot sequence degrades to once-per-programming).
    2. Reading port 0xEB must return the latched byte AND clock a new 0xFF
       exchange -- the divMMC streaming idiom.
    3. Trap classes differ: entry/exit points map AFTER their M1; 0x3D00-3DFF
       maps INSTANTLY (TR-DOS window; esxDOS's ROM-call trampoline RETs at
       0x3DFD and needs the fetch itself to read divRAM).
    4. CS writes (0xE7) must WAIT for an in-flight exchange: at the slow init
       clock, esxDOS reaches for CS while the preamble byte still shifts, and
       a mid-byte select desyncs the card's framing by the orphaned edges.
  tools/make_sd_image.py (MBR+FAT16 builder) and tools/make_test_tap.py
  (copyright-free autostart TAP) support `make esx`. Suite: 142 checks.
  Remaining: hardware wiring (6 pins) + recompile with esxdos.hex.
- 2026-08-03: first hardware boot -- banner and traps work on silicon, but
  the card conversation fails ("Detecting Devices..." forever). Three
  changes from the debugging session:
    1. SD diagnostics: in divMMC mode HEX1:0 shows the last SPI byte the
       CPU read from the card (FF = card never answers -> wiring; 01 =
       stuck in idle -> init protocol; other = further downstream) and
       HEX3:2 counts exchanges. HEX5:4 stay PS/2. Weak pull-up on MISO.
    2. divRAM wipe on divMMC reset: esxDOS keeps a warm-boot marker in
       divRAM, which survives soft reset in block RAM -- so every boot
       after the first silently skipped the banner and device detection
       straight into BASIC ("SW0 stopped working"). Reproduced in sim
       with the new `--reset-at N` harness flag; now a reset that enters
       divMMC mode zeroes all 64K first (~4.7 ms inside the reset).
       `make esx ... RESET=150` is the regression. Snapshot-armed resets
       never wipe. Corollary kept: a divMMC boot still clobbers the
       snapshot shadow, so SW[1] replay works until the first SW[0] boot,
       then needs reprogramming.
    3. SD model: deselect now aborts the operation in flight (a real
       card's CS behaviour); a mid-CMD18 host reset used to leave the
       model streaming stale blocks into the next boot's CMD0.
  Also learned the hard way: boottest/snaptest overwrite out/*.hex with
  test ROMs -- run `make esx`, never the boot_tb binary directly.
  Observed on hardware, expected: KEY[1] under the plain 48K ROM is a
  warm reset (the ROM's NMI-through-NMIADD=0 bug), and a SUCCESSFUL
  esxDOS boot also ends at (c) 1982 a few seconds after the module list
  -- that's the normal handover to BASIC, hooks armed.
