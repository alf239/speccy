# Which machine to build: the Soviet clone lineage

*2026-07-27 — outcome: **target the Scorpion ZS-256**, not a 48K.*

## Personal context

The two machines actually owned and used were both **Sergey Zonov designs** — his
entry-level machine and then his flagship. Both long gone, left behind in a country
move. No real 48K in the formative experience at all.

## Leningrad-1

Zonov, 1988. Probably the most-built Spectrum clone in the USSR. Its claim to fame
was minimalism: very low chip count on a single-sided board an amateur could etch
and stuff at home, which mattered enormously at the time.

With no way to obtain a Ferranti ULA behind the Iron Curtain, everything the ULA did
— video fetch, address multiplexing, RAS/CAS, sync generation, port decode,
interrupt — was rebuilt from Soviet 155/555/1533-series TTL (the K-prefixed
equivalents of 74/74LS/74ALS).

Rebuilt, but **not replicated**. Nobody was going to spend gates reproducing
Ferranti's exact contention pattern, so it was simply omitted, and interrupt and
frame timing came out subtly different. Hence the known failures of timing-sensitive
UK software — multicolour effects, floating-bus tricks.

## Scorpion ZS-256

Zonov again, with Andrey Larchenko, from the St Petersburg firm of the same name.
Near the top of what the Russian scene produced. From memory — **verify against
schematics before relying on any of this**:

- 256 KB RAM, paging extended past the 128K scheme via #1FFD alongside #7FFD
- switchable 3.5 / 7 MHz turbo
- Beta Disk / TR-DOS integrated rather than bolted on
- a service monitor/debugger resident in ROM — genuinely unusual for the era
- later SMUC boards added IDE and an RTC
- no memory contention

## Why this changes the target

Building a Pentagon- or Scorpion-class machine instead of a 48K **removes the single
hardest part of the project**. No contention, no floating bus, no snow — the fidelity
grind that makes a cycle-accurate 48K a months-long exercise is absent from these
machines *by design*.

Second advantage: they are discrete TTL with published schematics. That translates to
HDL almost mechanically — transcribing counters and decoders, not reverse-engineering
a gate array.

Third: it is the machine actually remembered, rather than a museum piece.

**Before writing from scratch**, survey existing cores (ZX-UNO and the MiST/MiSTer
Spectrum family support multiple clone timings). Not yet evaluated — worth an hour.

## Video output: what not to reproduce

Relevant because it determines the output stage design.

The notorious red bleeding is **not** a Sinclair defect — the Leningrad, which shares
no silicon with a Ferranti ULA, did it too. The culprit is composite PAL itself.
Three things stack up on red specifically:

1. **Luminance.** Y = 0.299R + 0.587G + 0.114B, so pure red carries ~30% of full
   luma. Red on black is a shape defined almost entirely by chroma, with barely any
   luma edge holding it together.
2. **Bandwidth.** PAL gives luma ~4.4 MHz but each colour-difference channel only
   ~1.3 MHz. Anything resolved from chroma alone is resolved at a third of the detail.
3. **The V axis.** Red sits essentially on V (R−Y) — precisely the axis PAL flips
   line-to-line to cancel phase errors. Every imperfection in that machinery surfaces
   first and worst in red. Hanover bars are a red phenomenon for the same reason.

The source side compounds it. A proper broadcast encoder low-passes the
colour-difference signals to ~1.3 MHz *before* modulating. A ULA — or a handful of
gates in Leningrad — slams the subcarrier phase to a new value at pixel boundaries,
generating chroma transitions at the 7 MHz pixel rate and driving them straight into
a 1.3 MHz filter. The visible smear is that filter ringing: a brick-wall filter fed a
step function. The RF modulator narrows everything again.

The rainbow shimmer on dithered patterns is the same disease inverted — luma detail
near 4.43 MHz mistaken for colour. Cross-colour and cross-luminance both follow from
carrying two signals on one wire and asking the receiver to guess.

Also relevant: at a 7 MHz pixel clock, a one-pixel checkerboard is a 3.5 MHz square
wave, sitting right under the 4.43 MHz subcarrier. **Single pixels were never
resolvable through a composite chain.** Attribute clash starts to look less like a
flaw and more like a rational response to a display that could never show 8×8 colour
detail anyway.

**Decision: drive RGB / VGA and none of this exists.** It is all in the output stage,
not the machine. If the artefacting is later missed, it belongs in a display shader —
keep it out of the core logic entirely.

## Related expectation management

The real 48K's virtues are as an object of study, not as a machine to use. Soviet
clones almost universally had real key-switch keyboards (a home builder had to source
a keyboard anyway; nobody fabricates a rubber mat on purpose), and the Scorpion was
pixel-crisp because it went out as RGB. A faithful FPGA reproduction on a modern LCD
will inevitably look *better* than the original ever did. That is fine, and worth
accepting deliberately rather than chasing.
