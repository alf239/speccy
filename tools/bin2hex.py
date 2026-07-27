#!/usr/bin/env python3
"""Binary ROM image -> hex file for $readmemh, padded to a full bank.

Usage: bin2hex.py 48.rom quartus/rom48.hex [size]
"""
import sys

src, dst = sys.argv[1], sys.argv[2]
size = int(sys.argv[3]) if len(sys.argv) > 3 else 16384

data = open(src, "rb").read()
if len(data) > size:
    sys.exit(f"{src} is {len(data)} bytes, does not fit in {size}")
data = data + bytes(size - len(data))
with open(dst, "w") as f:
    f.write("\n".join(f"{b:02X}" for b in data) + "\n")
print(f"{dst}: {len(data)} bytes ({src} was {len(open(src,'rb').read())})")
