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
| 1 | up | `GPIO[0]` |
| 2 | down | `GPIO[1]` |
| 3 | left | `GPIO[2]` |
| 4 | right | `GPIO[3]` |
| 6 | fire | `GPIO[4]` |
| 8 | ground | GND |

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
