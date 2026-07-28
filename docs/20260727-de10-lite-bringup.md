# Stage 1b: first bitstream on the DE10-Lite

*2026-07-27 — what can be done on the Windows box now, before the keyboard arrives.*

**Update, later the same day:** the Quartus project is now written and committed —
`quartus/speccy.qpf` opens directly, with all 121 pin assignments (cross-checked
against three independent DE10-Lite projects), the PLL as a hand-written altpll
instantiation, and timing constraints. The IP Catalog and pin-import steps below
are therefore **obsolete** — kept only in case the hand-written PLL is rejected by
a different Quartus version.

The default top is now the **full machine** (`de10_lite_speccy48`): with
`rom48.hex` present it boots the 48K ROM to the © 1982 message, no keyboard
needed. The static-screen ladder below is still the right first step if anything
misbehaves — switch `TOP_LEVEL_ENTITY` to `de10_lite_top` in the qsf.

## On the Windows box

```
git clone https://github.com/alf239/speccy
python tools/bin2hex.py 48.rom quartus/rom48.hex
```

Open `quartus/speccy.qpf`, Start Compilation, then Tools → Programmer → the
`.sof` in `quartus/output_files/`. Before flashing, run `make boot ROM=48.rom`
on the Mac and look at `out/boot.bmp` — if © 1982 renders in simulation, any
hardware failure is pins/PLL/monitor, not logic.

## What this stage delivers

A real Spectrum screen on a real monitor, with no CPU:

- screen contents from a `.mif`/`.hex` loaded into block RAM at configuration time
- border colour from `SW[2:0]`
- **the joystick working** — `LEDR[4:0]` per direction, Kempston port byte on `HEX1`/`HEX0`
- sync polarity on `SW[9]`, switchable at runtime

Everything up to here has been verified in simulation. This stage is the first
that can only be verified on hardware, so expect it to be the one that surprises.

## The joystick is the easy peripheral

Worth stating plainly, because it's the opposite of the keyboard situation: an
Atari-standard DE-9 stick is **five passive switches to ground**. It never drives a
voltage, so there is nothing to level shift. Enable weak pull-ups on the FPGA pins
and wire the connector straight in.

| DE-9 pin | Function | Wire to |
| --- | --- | --- |
| 1 | up | `GPIO[0]` — header pin 1 |
| 2 | down | `GPIO[2]` — header pin 3 |
| 3 | left | `GPIO[4]` — header pin 5 |
| 4 | right | `GPIO[6]` — header pin 7 |
| 6 | fire | `GPIO[10]` — header pin 13 (pin 9 is cobbler-grounded, see below) |
| 8 | ground | GND — header pin 12 or 30 |

Pin 9 is a second fire button on some sticks; Kempston has no bit for it.

Note that "Kempston compatible" is a property of the *interface*, not the stick —
any Atari-standard DE-9 joystick works, because the interface decides which bits
the switches land on. Five switches is exactly the right criterion.

Kempston reads as port 0x1F, active high, `000FUDLR`: bit 4 fire, bit 3 up, bit 2
down, bit 1 left, bit 0 right. The switches are active *low*, so the module inverts.
That inversion and the bit ordering are covered by `make joytest` — the mapping is
exactly the kind of thing that stays silently wrong until a game walks left when you
push right.

## Two things Quartus has to provide

**1. The PLL.** IP Catalog → Basic Functions → Clocks; PLLs and Resets → PLL →
ALTPLL. Name it `pll`.

| Setting | Value |
| --- | --- |
| inclk0 frequency | 50.000 MHz |
| c0 output | 14.000 MHz (multiply 7, divide 25) |
| `locked` output | enabled |

50 × 7/25 = 14 exactly — no fractional-N error, which is the happy accident that
makes this board suit a Spectrum.

**2. The pin assignments.** Use Terasic's own DE10-Lite assignment file from the
System CD rather than typing pin numbers by hand. Then add the joystick pull-ups:

```tcl
set_instance_assignment -name WEAK_PULL_UP_RESISTOR ON -to GPIO[0]
set_instance_assignment -name WEAK_PULL_UP_RESISTOR ON -to GPIO[1]
set_instance_assignment -name WEAK_PULL_UP_RESISTOR ON -to GPIO[2]
set_instance_assignment -name WEAK_PULL_UP_RESISTOR ON -to GPIO[3]
set_instance_assignment -name WEAK_PULL_UP_RESISTOR ON -to GPIO[4]
```

Terasic's file assigns pins for peripherals this design doesn't use (SDRAM,
accelerometer). Quartus warns and carries on — expected, not a problem.

## Generating the screen

The simulator emits the memory image, so the board and the testbench cannot drift
apart — same screen, one source of truth:

```bash
make run SCR=some_loading_screen.scr
```

```bash
./obj_dir/speccy_video_sim --scr some_loading_screen.scr --mem screen
```

That writes `screen.hex` (for `$readmemh`, which both Verilator and Quartus accept)
and `screen.mif` (fallback, if your Quartus declines to infer an initialised RAM
from `$readmemh` — then use the `ram_init_file` attribute noted in `rtl/vram.v`).

Put the file next to the project so Quartus finds it. With no `--scr` you get the
synthetic test pattern, which is arguably the better first target: its one-pixel
frame around the display window makes the exact edges measurable on a real screen.

## Files

- `rtl/de10_lite_top.v` — board wrapper. **Quartus only** — Verilator doesn't build
  it, because it instantiates a vendor PLL.
- `rtl/joystick.v` — synchroniser, debounce, Kempston mapping.

Add to the project: everything in `rtl/` except that Verilator-only concern, plus
the generated PLL.

## What you should see

In rough order of how early it fails:

1. `LEDR[9]` blinking at about 0.8 Hz — the 14 MHz clock is running
2. `LEDR[8]` lit — PLL locked
3. `LEDR[4:0]` following the joystick, `HEX1`/`HEX0` showing the Kempston byte
4. A picture, with `SW[2:0]` changing the border

Note that 1–3 work with no monitor attached at all, which makes them a genuine
bring-up ladder rather than a checklist: if the LEDs are wrong, don't waste time on
the video.

## Where this is most likely to go wrong

**The monitor won't lock.** This is the expected failure. Try `SW[9]` first — that
flips sync polarity without a rebuild. If it still won't lock, the suspects are the
sync *position* within the blanking interval and the blank/border split, both of
which are flagged as uncertain in `rtl/video_timing.v` and are parameters precisely
so they can be tuned here. The totals (448 × 312) and the display window (256 × 192)
are right and should not be touched.

Simulation proved the doubling is correct. It cannot prove a monitor likes where the
sync pulse sits — that is what this stage is for.

**Output is 352 × 592 visible**, which is not a standard mode, so monitors may
letterbox it or report an odd resolution. That's normal for a scandoubled Spectrum.

**No `.mif` found** shows up as a black or garbage display with everything else
working — check the LEDs first to distinguish this from a video failure.

## Postscript, evening of 2026-07-27: it works

First-try bring-up. © 1982 Sinclair Research Ltd on the VGA monitor; PLL, pin
table and 50.08 Hz all accepted unmodified. The sync-position guesswork above
turned out fine — the image sits a little off-centre on the panel, which the
monitor's auto-adjust should absorb; tune `H_SYNC_OFF`/`V_SYNC_OFF` only if it
doesn't.

## Beeper output circuit

One divider, one cap, into powered speakers or line-in. Values are non-critical
(anything within 2× works); the jack is best scavenged from dead wired earbuds,
using their cable. Tip and ring tied together for mono.

```
GPIO[32] --[ R1 10k ]--+--[ C2 1uF ]--> tip+ring of 3.5mm jack
                       |
                 [ R2 1k ]    (+ optional C1 10n across R2)
                       |
GND -------------------+--------------> sleeve
```

- R1/R2: divide 3.3 V down to ~0.3 Vpp — line-level polite. Ratio 5:1 to 20:1
  all fine, it is just volume.
- C2: DC block. 1 uF into a 10k line input passes >16 Hz; 100 nF also fine
  (rolls off below ~160 Hz — the beeper has no bass to lose).
- C1: ~16 kHz low-pass. Cosmetic for the beeper, but it becomes the DAC when
  phase 2 does AY via sigma-delta — fit it now and the audio path never changes.

`GPIO[32]` is header pin 37 (cobbler hole `P26`), and mind pins 11/29, which
are 5 V and 3.3 V outputs. Headphones directly off the divider will be
very quiet; powered input is the intended load.

## PS/2 keyboard wiring (stage 3e)

The FPGA side is `GPIO[26]` = clock, `GPIO[28]` = data (header pins 31/33 —
both on the odd column, like every other peripheral). Both are receive-only inputs with weak pull-ups
in the qsf. Three rules:

1. **Power the keyboard from the header's 5 V pin (pin 11)**, ground at 12 or
   30. PS/2 keyboards are 5 V devices; most won't run from 3.3 V.
2. **Put a 2.2 kΩ resistor in series with clock and with data** (the joystick
   kit has five). The keyboard pulls these lines to 5 V and MAX 10 is not 5 V
   tolerant; the resistor limits the input clamp-diode current to ~0.5 mA,
   which turns "out of spec" into "out of spec, safely".
3. **Clock/data swapped is harmless** — both are inputs, nothing can be
   damaged. If the keyboard is powered but no keys register, swap the two
   signal wires and try again. This is the cheapest debugging step in the
   whole project.

### Sacrificial PS/2-to-USB adapter as the socket

A passive adapter donates a female mini-DIN-6 with a short pigtail. Do NOT
trust wire colours or pinout diagrams read off the female face (it mirrors
the male drawing — the classic trap). Instead, identify electrically:

- Beep from the **USB plug's outer two contacts** (VBUS and GND) back to the
  DIN side: that finds 5 V and ground with certainty.
- The remaining two connected pins are clock and data. Which is which
  doesn't matter (rule 3): guess, and swap if dead.

Note: only ~4 of the 6 DIN pins will have continuity to anything — pins 2
and 6 are unconnected in a keyboard adapter. That's expected.

### What the simulation already proves

`make boot ROM=48.rom TYPE='...'` drives real scancode traffic into the PS/2
pins of the full machine. The testbench has typed `10 P"hello"` + `R` + Enter
into the actual ROM: program entered, run, `hello` printed, `0 OK, 10:1`.
So when the physical keyboard misbehaves, the suspects are wiring and the
5 V domain, not the RTL.

Timing quirk found doing that: the ROM's KSTATE machinery refuses to
re-register a key within ~5 frames of its release — a real Spectrum
behaviour, not a bug. Double letters ("ll") typed faster than that lose the
second press. Humans never type that fast; testbenches do.

## GPIO header crib sheet (verified)

From the DE10-Lite manual's Table 3-7 (via UF EEL 3701's pin reference, which
includes the header photo), cross-checked against the FPGA balls in our qsf.
Odd pins are one column, even pins the other; pin 1 is marked on the
silkscreen.

```
 PIN_V10  GPIO[0]    1 |  2  GPIO[1]   PIN_W10
 PIN_V9   GPIO[2]    3 |  4  GPIO[3]   PIN_W9
 PIN_V8   GPIO[4]    5 |  6  GPIO[5]   PIN_W8
 PIN_V7   GPIO[6]    7 |  8  GPIO[7]   PIN_W7
 PIN_W6   GPIO[8]    9 | 10  GPIO[9]   PIN_V5
          ** 5V **  11 | 12  ** GND **
 PIN_W5   GPIO[10]  13 | 14  GPIO[11]  PIN_AA15
 PIN_AA14 GPIO[12]  15 | 16  GPIO[13]  PIN_W13
 PIN_W12  GPIO[14]  17 | 18  GPIO[15]  PIN_AB13
 PIN_AB12 GPIO[16]  19 | 20  GPIO[17]  PIN_Y11
 PIN_AB11 GPIO[18]  21 | 22  GPIO[19]  PIN_W11
 PIN_AB10 GPIO[20]  23 | 24  GPIO[21]  PIN_AA10
 PIN_AA9  GPIO[22]  25 | 26  GPIO[23]  PIN_Y8
 PIN_AA8  GPIO[24]  27 | 28  GPIO[25]  PIN_Y7
          **3.3V**  29 | 30  ** GND **
 PIN_AA7  GPIO[26]  31 | 32  GPIO[27]  PIN_Y6
 PIN_AA6  GPIO[28]  33 | 34  GPIO[29]  PIN_Y5
 PIN_AA5  GPIO[30]  35 | 36  GPIO[31]  PIN_Y4
 PIN_AB3  GPIO[32]  37 | 38  GPIO[33]  PIN_Y3
 PIN_AB2  GPIO[34]  39 | 40  GPIO[35]  PIN_AA2
```

### This project's wiring, by physical pin

**Everything lives on the odd column** — one physical row on the header, one
row on the breadboard — with a single ground wire to the even side:

| Signal | GPIO | Header pin |
| --- | --- | --- |
| Joystick up | GPIO[0] | **1** |
| Joystick down | GPIO[2] | **3** |
| Joystick left | GPIO[4] | **5** |
| Joystick right | GPIO[6] | **7** |
| Joystick fire | GPIO[10] | **13** |
| Keyboard 5 V supply | — | **11** |
| PS/2 clock (via 2.2 kΩ) | GPIO[26] | **31** |
| PS/2 data (via 2.2 kΩ) | GPIO[28] | **33** |
| Beeper out | GPIO[32] | **37** |
| Ground for everything | — | **12** or **30** (even side) |

Read down the odd column: five joystick pins, then 5 V exactly where the
keyboard needs it, a gap, PS/2 pair, gap, beeper. The remaining hazards:
**pin 11 is 5 V immediately after the joystick block** — overshoot by one odd
position and a switch meets 5 V — and **3.3 V (29) adjoins PS/2 clock (31)**.
Count from the silkscreened pin 1; the header has no key.

## Pi T-cobbler translation (labels are Pi names — they lie)

The breakout is passive and 1:1: J1 = odd header pins 1..39 top-to-bottom,
J2 = even pins 2..40. But the silkscreen carries Raspberry Pi signal names,
which are disinformation against the DE10-Lite. Tape over J1 and relabel.

| DE10-Lite pin | Our signal | Cobbler hole (J1 row) says |
| --- | --- | --- |
| 1 | Joystick up | row 1, `+3.3V` |
| 3 | Joystick down | row 2, `P02(SDA1)` |
| 5 | Joystick left | row 3, `P03(SCL1)` |
| 7 | Joystick right | row 4, `P04` |
| 13 | Joystick fire | row 7, `P27` |
| 11 | 5 V → keyboard | row 6, `P17` |
| 31 | PS/2 clock (2.2 kΩ) | row 16, `P06` |
| 33 | PS/2 data (2.2 kΩ) | row 17, `P13` |
| 37 | Beeper | row 19, `P26` |

**Ground** (the only wires on J2): DE10-Lite pins 12 and 30 =
**J2 row 6 (`P18`)** and **J2 row 15 (`GND`)**. Do NOT use the `GND` at the
top of J2 — that is Pi pin 6 = DE10-Lite `GPIO[5]`, a live FPGA I/O.
(Caught by cross-checking the Terasic manual against the cobbler; the row-15
label is truthful only because the Pi coincidentally grounds pin 30 too.)

**Orientation proof before wiring anything** (asymmetric, so a reversed
ribbon cannot fake both): hole `P17` (J1 row 6) reads 5 V and hole `P05`
(J1 row 15) reads 3.3 V. Wrong readings = flip the ribbon at the board end;
the cobbler end is keyed.

### This particular adapter's pigtail (beeped out 2026-07-28)

| Wire | mini-DIN pin | Function | Connection |
| --- | --- | --- | --- |
| red | 4 | Vcc | straight to J1 row 6 (`P17`) = 5 V |
| black | 3 | GND | straight to J2 row 6 (`P18`) |
| white | 5 | clock | 2.2 kΩ to J1 row 16 (`P06`) |
| green | 1 | data | 2.2 kΩ to J1 row 17 (`P13`) |

Identified by continuity, not colour convention (though it happens to match
it). Keyboard lock-LEDs flashing at power-on = free proof the 5 V path works
before any key is pressed.

## PS/2 debugging without a scope

The board displays its own diagnosis on the 7-segment displays:

| Display | Shows |
| --- | --- |
| HEX3:HEX2 | last scancode received |
| HEX4 | raw clock-edge counter — spins on ANY line activity |
| HEX5 | frame-error count |

Interpretation ladder:

1. **Plug in / power-cycle the keyboard.** Its lock-LEDs should flash once
   (self-test), and HEX3:2 should read **AA** — the BAT completion code every
   keyboard transmits unprompted ~0.5-0.75 s after power. No key needed.
2. AA showing → everything works; press A and expect 1C, release adds F0 1C.
3. HEX4 moving but HEX3:2 dead / HEX5 counting → line is alive but garbled:
   clock/data swapped (swap the two resistored wires) or bad contact.
4. Nothing moves anywhere, keyboard LEDs never flash → keyboard has no power
   or is dead. Check 5 V at the keyboard end. ~4.7 V on the header is normal
   (USB supply minus a protection diode) and within PS/2 tolerance.

Who drives the clock: **the keyboard, always** — the host only inhibits by
holding it low. A silent clock line therefore always means a dead or
unpowered device, never a host-side fault.

Note: our receive-only host cannot set the lock LEDs, so Caps Lock's LED
staying dark during use is expected, not a fault.


## The cobbler has a ground plane (discovered the fun way)

With nothing connected, the Kempston byte read 0x10 — fire held. Fire was
header pin 9, whose cobbler hole is labelled GND, and the board ties ALL
Pi-ground holes (Pi pins 6, 9, 14, 20, 25, 30, 34, 39) into one pour. Since
DE10-Lite pin 30 is real ground, the entire net is grounded — including five
FPGA inputs and, until this was caught, the beeper output.

Consequences, while the cobbler is fitted:

- **GPIO[8], [11], [17], [22], [29], [34] (pins 9, 14, 20, 25, 34, 39) are
  grounded — never assign them as signals.** They stay tri-stated in the top.
- Joystick fire moved to GPIO[10] (pin 13, hole `P27`); beeper to GPIO[32]
  (pin 37, hole `P26`).
- Silver lining: every GND-labelled hole on the cobbler is now genuinely
  ground (via pin 30), so ground taps are plentiful.
- The 5 V pour commons GPIO[1] and GPIO[3] (pins 2/4) together — keep them
  unused. The 3.3 V pour commons GPIO[0] and GPIO[14] (pins 1/17): harmless
  for joystick-up on pin 1, since GPIO[14] is tri-stated and just rides along.
- The corner power pins on the cobbler's crossbar connect to those floating
  pours, NOT to real rails — don't power anything from them.

A plain 1:1 breakout without pours would not have this issue, but the pour
also isn't worth abandoning the board over: everything fits on the remaining
pins with room to spare.
