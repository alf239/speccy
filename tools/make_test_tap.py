#!/usr/bin/env python3
"""Generate a small, genuine, copyright-free .TAP: an autostarting BASIC
program that sets the border and prints a message. Used to prove the whole
esxDOS tape path in simulation.

Usage: make_test_tap.py OUT.TAP
"""
import struct
import sys


def num(n):
    """Inline number: ASCII digits + 0x0E marker + 5-byte binary form."""
    return str(n).encode() + b"\x0E\x00\x00" + struct.pack("<H", n) + b"\x00"


def line(no, content):
    return struct.pack(">H", no) + struct.pack("<H", len(content) + 1) + content + b"\x0D"


prog = b""
prog += line(10, b"\xE7" + num(2))                       # BORDER 2
prog += line(20, b"\xF5\x22SD BOOT OK VIA ESXDOS\x22")   # PRINT "..."
prog += line(30, b"\xF5\x22HELLO FROM THE FPGA\x22")     # PRINT "..."

def block(flag, payload):
    body = bytes([flag]) + payload
    par = 0
    for b in body:
        par ^= b
    body += bytes([par])
    return struct.pack("<H", len(body)) + body


hdr = struct.pack("<B10sHHH", 0, b"hello     ", len(prog), 10, len(prog))
tap = block(0x00, hdr) + block(0xFF, prog)

out = sys.argv[1] if len(sys.argv) > 1 else "out/hello.tap"
open(out, "wb").write(tap)
print(f"wrote {out}: {len(prog)} bytes of BASIC, autostart line 10")
