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

## The other road: a from-scratch FPGA board

A custom board (FPGA + SDRAM + video DAC + config flash, ZX outline, no
DE10-Lite) is the aesthetically pure endgame — and a different, much larger
project: fine-pitch parts, power sequencing, a new bring-up debt, and it
obsoletes the board we know works. It also pairs naturally with phase 2's
SDRAM appetite (128K, Scorpion). Verdict: **phase 3.** The carrier gets us
into the case this year; the custom board is what the carrier's success
earns.

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

Feasible, cheap, and the right shape of project: one evening of RTL, two of
KiCad, and the machine trades its cobbler hat for a chassis it historically
belongs in. The keyboard — the part that looked like it needed a
"controller" — is the most authentic interface of the whole build: thirteen
wires and a matrix, scanned the way Ferranti intended.
