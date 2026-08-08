#!/usr/bin/env python3
"""AY register log -> note events, per channel, plus a MIDI file.

Input: lines of "frame reg val" (from boot_tb --aylog). Reconstructs the
register state frame by frame and emits, per channel, the audible note
stream: tone period -> frequency -> nearest note, with volume, envelope
and noise flags. Consecutive frames of the same note merge into events.

Usage: ay2notes.py aylog.txt [--midi out.mid] [--from FRAME] [--to FRAME]

The AY clock is 1.75 MHz (Pentagon convention): f = 1750000 / (16 * TP).
"""

import sys
import math
import struct

NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


def note_name(midi):
    return f"{NAMES[midi % 12]}{midi // 12 - 1}"


def period_to_midi(tp):
    if tp == 0:
        tp = 1
    f = 1750000.0 / (16.0 * tp)
    if f < 20 or f > 12000:
        return None, f
    midi = round(69 + 12 * math.log2(f / 440.0))
    return midi, f


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    path = sys.argv[1]
    midi_out = None
    f_lo, f_hi = 0, 10**9
    args = sys.argv[2:]
    while args:
        a = args.pop(0)
        if a == "--midi":
            midi_out = args.pop(0)
        elif a == "--from":
            f_lo = int(args.pop(0))
        elif a == "--to":
            f_hi = int(args.pop(0))

    # Replay the log into per-frame register snapshots.
    regs = [0] * 16
    writes = []                              # (frame, reg, val)
    for line in open(path):
        fr, rg, vl = (int(x) for x in line.split())
        if f_lo <= fr <= f_hi:
            writes.append((fr, rg, vl))
    if not writes:
        sys.exit("no AY writes in range")

    first, last = writes[0][0], writes[-1][0]
    print(f"{len(writes)} register writes over frames {first}..{last} "
          f"({(last - first) / 50.0:.1f} s)")

    # Walk frames; for each, apply writes then snapshot channel state.
    events = {ch: [] for ch in "ABC"}        # (start, end, midi, vol, flags)
    cur = {ch: None for ch in "ABC"}
    wi = 0
    for fr in range(first, last + 1):
        while wi < len(writes) and writes[wi][0] <= fr:
            _, rg, vl = writes[wi]
            regs[rg] = vl
            wi += 1
        for ci, ch in enumerate("ABC"):
            tp = ((regs[2 * ci + 1] & 0x0F) << 8) | regs[2 * ci]
            vol = regs[8 + ci] & 0x0F
            env = bool(regs[8 + ci] & 0x10)
            tone_on = not (regs[7] >> ci) & 1
            noise_on = not (regs[7] >> (ci + 3)) & 1
            audible = (env or vol > 0) and (tone_on or noise_on)
            midi, freq = period_to_midi(tp) if tone_on else (None, 0)
            key = (midi, env, noise_on) if audible else None
            state = cur[ch]
            if state and state[2] == key:
                continue
            if state:
                events[ch].append((state[0], fr, state[1], state[2]))
            cur[ch] = (fr, (vol, env, noise_on, freq), key) if audible else None
    for ch in "ABC":
        if cur[ch]:
            events[ch].append((cur[ch][0], last + 1, cur[ch][1], cur[ch][2]))

    for ch in "ABC":
        evs = [e for e in events[ch] if e[3] and e[3][0] is not None]
        if not evs:
            print(f"\nchannel {ch}: no tonal content")
            continue
        lo = min(e[3][0] for e in evs)
        hi = max(e[3][0] for e in evs)
        print(f"\nchannel {ch}: {len(evs)} notes, range "
              f"{note_name(lo)}..{note_name(hi)}")
        for s, e, (vol, env, noise, freq), (midi, *_ ) in evs[:400]:
            dur = (e - s) * 20
            flags = ("env " if env else "") + ("noise" if noise else "")
            print(f"  f{s:5d} {note_name(midi):4s} {freq:7.1f}Hz "
                  f"{dur:5d}ms v{vol:2d} {flags}")
        if len(evs) > 400:
            print(f"  ... {len(evs) - 400} more")

    if midi_out:
        write_midi(midi_out, events, first)
        print(f"\nwrote {midi_out}")


def write_midi(path, events, first):
    # One track per channel; 1 tick = 1 frame (20 ms), tempo 50 frames/beat.
    def vlq(n):
        out = [n & 0x7F]
        n >>= 7
        while n:
            out.append(0x80 | (n & 0x7F))
            n >>= 7
        return bytes(reversed(out))

    tracks = []
    for ci, ch in enumerate("ABC"):
        data = bytearray()
        t = 0
        data += vlq(0) + bytes([0xFF, 0x51, 0x03]) + struct.pack(">I", 1000000)[1:]
        for s, e, (vol, env, noise, freq), key in events[ch]:
            if not key or key[0] is None:
                continue
            midi = max(0, min(127, key[0]))
            vel = 40 + vol * 5
            data += vlq(s - first - t) + bytes([0x90 | ci, midi, min(127, vel)])
            data += vlq(e - s) + bytes([0x80 | ci, midi, 0])
            t = e - first
        data += vlq(0) + bytes([0xFF, 0x2F, 0x00])
        tracks.append(bytes(data))

    with open(path, "wb") as f:
        f.write(b"MThd" + struct.pack(">IHHH", 6, 1, len(tracks), 50))
        for tr in tracks:
            f.write(b"MTrk" + struct.pack(">I", len(tr)) + tr)


if __name__ == "__main__":
    main()
