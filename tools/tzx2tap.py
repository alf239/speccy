#!/usr/bin/env python3
"""TZX -> TAP for standard-speed tapes (what esxDOS's tape layer loads).

Usage: tzx2tap.py IN.TZX [OUT.TAP]

Converts standard-speed data blocks (and turbo blocks whose timings are
actually standard). Skips metadata. Refuses tapes that genuinely need a
custom loader -- those bytes only make sense as audio timing, which is
stage-EAR-line material, not TAP material.
"""

import struct
import sys

STD = dict(pilot=2168, sync1=667, sync2=735, bit0=855, bit1=1710)


def convert(data):
    if data[:7] != b"ZXTape!":
        raise ValueError("not a TZX file")
    i = 10
    out = bytearray()
    blocks = kept = 0
    while i < len(data):
        bid = data[i]; i += 1
        blocks += 1
        if bid == 0x10:                                  # standard speed
            _, ln = struct.unpack_from("<HH", data, i)
            payload = data[i + 4:i + 4 + ln]
            out += struct.pack("<H", ln) + payload
            i += 4 + ln
            kept += 1
        elif bid == 0x11:                                # turbo: maybe standard
            (pilot, sync1, sync2, bit0, bit1, _pl, _used, _pause,
             l0, l1, l2) = struct.unpack_from("<HHHHHHBH3B", data, i)
            ln = l0 | l1 << 8 | l2 << 16
            timings = dict(pilot=pilot, sync1=sync1, sync2=sync2,
                           bit0=bit0, bit1=bit1)
            near = all(abs(timings[k] - STD[k]) <= 40 for k in STD)
            if not near:
                raise ValueError(
                    f"turbo loader block ({timings}) -- needs real tape audio")
            out += struct.pack("<H", ln) + data[i + 18:i + 18 + ln]
            i += 18 + ln
            kept += 1
        elif bid == 0x20: i += 2                          # pause/stop
        elif bid == 0x21: i += 1 + data[i]                # group start
        elif bid == 0x22: pass                            # group end
        elif bid == 0x30: i += 1 + data[i]                # text
        elif bid == 0x31: i += 2 + data[i + 1]            # message
        elif bid == 0x32: i += 2 + struct.unpack_from("<H", data, i)[0]
        elif bid == 0x33: i += 1 + 3 * data[i]            # hardware info
        elif bid == 0x35: i += 0x14 + struct.unpack_from("<I", data, i + 0x10)[0]
        elif bid == 0x5A: i += 9                          # glue
        else:
            raise ValueError(f"block 0x{bid:02X} (custom loader/control flow)")
    return bytes(out), kept, blocks


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    src = sys.argv[1]
    dst = sys.argv[2] if len(sys.argv) > 2 else src.rsplit(".", 1)[0] + ".tap"
    tap, kept, total = convert(open(src, "rb").read())
    open(dst, "wb").write(tap)
    print(f"{dst}: {kept} data blocks from {total} TZX blocks, {len(tap)} bytes")


if __name__ == "__main__":
    main()
