# The machine gets a body: a ZX-48 form-factor carrier board

*2026-08-03 — evaluation. The prototype works; the question is whether it can
move out of the cobbler-and-jumper era and into a modern ZX Spectrum case
with a Cherry MX keyboard, on a PCB matching the original chassis.*

## The ask, decomposed

A modern reproduction ZX case (Mechtrum-style) provides three things: the
original motherboard mounting geometry, a mechanical keyboard where the
rubber mat used to be, and apertures roughly where Sinclair put them. It
expects a board shaped like a ZX Spectrum issue board. We have a DE10-Lite
wearing a prototype hat.

The move: a **carrier PCB** in the original board's outline that hosts the
unmodified DE10-Lite as a daughterboard, and absorbs everything currently
done by the cobbler, the prototype board, and the flying wires.

## The keyboard is the easy part (surprisingly)

**Confirmed for the case in hand:** its MX keyboard is electrically the
original interface — the two membrane tails, **8 scan rows + 5 columns**,
a bare 8×5 switch matrix, passive, no controller anywhere, made to plug
into the original Speccy's keyboard sockets. Spare original sockets are
already in the parts drawer (salvage from a board whose own were fine),
so the carrier's keyboard connectors are the genuine article.

So the "ZX-style keyboard controller" the plan called for is... two FFC/tail
connectors and 13 GPIOs. A ~50-line matrix scanner in Verilog drives the
8 row lines and samples the 5 columns into the same `key_matrix[39:0]` the
machine already consumes (OR-ed with PS/2 and the autokeys, which all stay).
This is *less* hardware than the PS/2 path — it is precisely how the real
ULA read the real membrane. The DIN-5 socket can stay as a secondary port.

## What lives on the carrier

| Block | Parts | Notes |
| --- | --- | --- |
| DE10-Lite socket | 2×20 receptacle + standoffs | board hangs off its GPIO header, unmodified, reprogrammable in place |
| Keyboard | 5-way + 8-way tail connectors | + optional pull-up pack; scanner in RTL |
| Storage | push-push microSD socket | retires the breakout; SPI routed short, over ground |
| Joystick | DE-9 male, side aperture | Kempston (5 GPIOs, as today) |
| Joystick 2 | DE-9 male, passive | Sinclair port 1: five switches tapping the keyboard matrix traces (6-7-8-9-0), the Interface 2 way — **zero GPIOs, zero logic** |
| Audio | 3.5 mm jack + RC, optional small speaker + transistor | beeper as today; **EAR/MIC jacks land here too — phase-2 tape input becomes a connector, not a rework** |
| Power | case DC jack position → 5 V | polyfuse + reverse-polarity diode, feeds the DE10-Lite's 5 V |
| Keyboard legacy | DIN-5 socket (optional) | the stickered AT board keeps working |

## The aperture problem (and its stock answer)

The DE10-Lite's VGA, USB-Blaster and power connectors sit on its own edges.
Orient the daughterboard connectors-rearward and they exit through the
**expansion-slot aperture** — the widest hole Sinclair ever gave us — and
Mechtrum-style cases have a *replaceable rear panel* explicitly intended
for this kind of modification (composite/VGA mods on real boards). VGA and
programming cable out the back, exactly where the Interface 2 used to hang.

## Outline and mounting: don't measure, copy

The original issue-3 outline, hole positions and aperture cutouts are
already captured by the **Harlequin** project (48K clone PCBs built to fit
original cases) and several published issue-board recreations. Lift the
board outline + mounting holes as DXF into KiCad rather than measuring a
case with calipers. Verify against the *actual* case before ordering — 
repro cases are copies of copies, and bosses drift.

## Routing reality check

The fastest signal leaving the DE10-Lite is 7 MHz SPI over a few
centimetres. Everything else is a keyboard matrix, DC power and audio.
This is a **2-layer board, hand-routable in an evening**, with exactly two
disciplines: keep SD short with ground nearby, and keep the 5 V world
(DIN-5 keyboard) away from the 3.3 V world with the same 2.2 kΩ series
resistors used today. The routing is not the project; the *mechanical*
correctness (outline, holes, aperture alignment, stack height) is the
project. Plan on a paper/cardboard fit-check print of the layout before
sending rev A anywhere.

**Stack height is the one genuine risk**: DE10-Lite + socket + its VGA
connector is ~25 mm tall. Original boards with the RF modulator were about
that. Measure the case's internal clearance over the motherboard area
before finalising which way up the daughterboard sits.

## The other road: a from-scratch FPGA board (promoted after review)

Initially parked as phase 3 — then two facts moved it within reach:

**1. The DE10-Lite's GPIO header is wired straight to MAX 10 pins.** No
buffers, no shifters (hence the no-5V rule). Everything this project uses
on that board is: the FPGA, a 50 MHz oscillator, a VGA resistor ladder,
power regulation, and connectors. Our RTL is 100% portable within the
MAX 10 family — a custom board changes *only the .qsf pin table*. Same
M9K, same internal-flash instant-on configuration, same
`SINGLE IMAGE WITH ERAM` memory-init trick. The FPGA-side risk of a
custom board is a pin remap.

**2. The right part exists in a friendly package: 10M50SAE144.** Same
50K LE and — the binding constraint — the same 1,638 Kb of M9K as the
DE10-Lite's chip, but:
- **EQFP-144** (0.5 mm quad flat, exposed pad): routine for any PCBA
  service, even hand-reworkable — no BGA anxieties
- **Single supply** ("SA"): internal regulators, so the board needs one
  3.3 V rail instead of the DE10-Lite's multi-rail arrangement
- ~101 I/Os against our ~50-pin appetite
- ~£40–60, stocked — the dominant cost of the whole board

### What the custom board carries beyond the carrier's list

| Block | Parts | Notes |
| --- | --- | --- |
| FPGA | 10M50SAE144C8G | C8 slowest grade is 5× faster than we need |
| Power | 5 V in → 3.3 V buck + analog filtering | single rail thanks to SA |
| Clock | 50 MHz 3.3 V oscillator | PLL settings unchanged |
| Config | nothing! | MAX 10 internal flash, instant-on |
| JTAG | 10-pin header | needs a standalone USB-Blaster dongle (~£10) — the one we use today lives on the DE10-Lite |
| VGA | resistor DAC, 4:4:4 | copied from the DE10-Lite schematic |
| SDRAM | W9825G6KH (32 MB, TSOP-54, ~£2) | **phase 2 lands on copper now** — 128K/Scorpion's memory, routed once |

Board: 4-layer (signal/GND/power/signal), which makes SDRAM routing and
decoupling boring instead of clever. Rev A cost, assembled, roughly
£150–250 for a couple of boards — against the carrier's £20, buying
sovereignty, SDRAM, and silence from the cobbler forever.

### Bring-up ladder (each rung small)

power rails → JTAG chain detected → LED blink → pin-remapped speccy.qsf →
© 1982 → esxDOS. Steps 4–6 are known-good logic; the new debt is rungs 1–3.

The DE10-Lite retires to the bench as the simulation-adjacent lab mule —
every RTL change still proves itself there before touching the case.

## Open questions before schematic capture

1. ~~Keyboard interface~~ — **resolved**: standard ZX tails into standard
   ZX sockets, of which spares are in hand.
2. **Case internal clearance** over the board area (stack height).
3. **Power entry**: reuse the case's DC aperture position, or feed through
   the rear panel next to VGA?
4. ~~Second joystick port~~ — **resolved**: it's passive matrix taps
   (Interface 2 style), so fitting it costs traces, not pins. Fit it.

## Order of work

1. Confirm case model; obtain/measure keyboard pinout and case clearances
2. RTL first, as always: matrix scanner + testbench, proven on the bench
   with jumper wires to the case keyboard *before* any PCB exists
3. KiCad: outline from Harlequin DXF, schematic, 2-layer layout
4. Cardboard fit check in the actual case
5. Rev A (~£20 for five boards), assemble one, live with it
6. Rev B fixes what living with it finds

## Verdict

Both roads reach the case. The carrier is the £20 weekend; the custom
board is the real machine — PCB + PCBA, MAX 10 in EQFP-144 with the tiny
parts soldered by the fab, SDRAM waiting for phase 2, and nothing on the
board we didn't put there. Since the RTL ports with a pin table and the
chip is stocked in a package a hobbyist can rework, the custom board is
**no longer a different kind of project — just a bigger one**, and it's
the one worth doing. The keyboard verdict stands either way: thirteen
wires and a matrix, scanned the way Ferranti intended.
