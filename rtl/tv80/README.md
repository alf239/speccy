# TV80 (vendored)

Z80-compatible synthesizable Verilog core by Guy Hutchison, itself based on
Daniel Wallner's VHDL T80. MIT licence — see `LICENSE`, and the notice at the
top of every file.

- Upstream: https://github.com/hutch31/tv80
- Vendored from commit `66a131c38d05ef58b3d8c4f1507a72e6e4aa5d65` (2026-05-11)
- Files taken: `rtl/core/{tv80s,tv80_core,tv80_alu,tv80_mcode,tv80_reg}.v`
  (`tv80n.v` is the negative-edge variant and the `sd_*` files are a
  self-distributing memory interface; neither is used here)

## Local changes

Two, both in `tv80s.v`, both marked `LOCAL CHANGE` in the source:

1. The `cen` clock enable is exposed as an input port instead of being tied
   to 1. Upstream's `tv80_core` already implements the enable; the wrapper
   just didn't bring it out. This is how the core runs at 3.5 MHz (7 MHz
   later, for the Scorpion turbo) off the single 14 MHz machine clock.

2. The bus-signal block is gated with the same `cen`. Ungated, it deasserts
   `mreq_n`/`rd_n` partway through a multi-clock T2, so memory data is gone
   before the core's T3 sample point — every opcode fetch reads the idle bus
   value 0xFF, which is RST 38h, and the CPU livelocks pushing its own PC.
   Found by the CPU smoke test on first run; the trace of SP marching down
   from 0xFFFE while fetching 0x0038 forever is unambiguous in hindsight.

Everything else is byte-identical to upstream. Lint warnings from these files
are waived in `tv80.vlt` rather than fixed, to keep the diff against upstream
reviewable.
