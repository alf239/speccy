#!/usr/bin/env python3
"""Build a FAT16 SD-card image (MBR + one partition) for esxDOS.

Usage: make_sd_image.py OUT.IMG [--tree DIR] [--add SRC=DEST8.3] ...

--tree copies DIR's top-level files and one level of subdirectories (8.3
names only -- anything longer is skipped with a warning). --add places a
single file at a path like GAMES/HELLO.TAP (directories auto-created).

Geometry: 16 MB, partition at LBA 2048, FAT16, 2 KB clusters -- small,
valid, and comfortably inside what esxDOS mounts.
"""

import os
import struct
import sys

SEC = 512
TOTAL_SECS = 32768                 # 16 MB
PART_START = 2048
PART_SECS = TOTAL_SECS - PART_START
SEC_PER_CLUS = 4
RESERVED = 1
NFATS = 2
ROOT_ENTRIES = 512
ROOT_SECS = ROOT_ENTRIES * 32 // SEC
FAT_SECS = 30                      # covers ~7.6K clusters at 2 bytes each

DATE = ((2026 - 1980) << 9) | (8 << 5) | 2
TIME = (12 << 11)


class Fat16:
    def __init__(self):
        self.img = bytearray(TOTAL_SECS * SEC)
        self.fat = [0] * (FAT_SECS * SEC // 2)
        self.fat[0] = 0xFFF8
        self.fat[1] = 0xFFFF
        self.next_clus = 2
        self.root = []             # list of 32-byte entries
        self.data_start = PART_START + RESERVED + NFATS * FAT_SECS + ROOT_SECS

    def alloc_chain(self, nclus):
        first = self.next_clus
        for i in range(nclus):
            c = self.next_clus + i
            self.fat[c] = c + 1 if i < nclus - 1 else 0xFFFF
        self.next_clus += nclus
        return first

    def clus_off(self, clus):
        return (self.data_start + (clus - 2) * SEC_PER_CLUS) * SEC

    def write_data(self, data):
        nclus = max(1, -(-len(data) // (SEC_PER_CLUS * SEC)))
        first = self.alloc_chain(nclus)
        off = self.clus_off(first)
        self.img[off:off + len(data)] = data
        return first, nclus

    @staticmethod
    def name83(name):
        name = name.upper()
        if "." in name:
            base, ext = name.rsplit(".", 1)
        else:
            base, ext = name, ""
        if len(base) > 8 or len(ext) > 3 or not base:
            return None
        return base.ljust(8) + ext.ljust(3)

    @staticmethod
    def entry(name11, attr, clus, size):
        return struct.pack("<11sB10xHHHI", name11.encode(), attr,
                           TIME, DATE, clus, size)

    def add_file(self, entries, name, data):
        n = self.name83(name)
        if n is None:
            print(f"  skip (not 8.3): {name}")
            return
        clus, _ = self.write_data(data) if data else (0, 0)
        entries.append(self.entry(n, 0x20, clus if data else 0, len(data)))

    def add_dir(self, parent_entries, name, parent_clus):
        n = self.name83(name)
        if n is None:
            print(f"  skip dir (not 8.3): {name}")
            return None
        clus = self.alloc_chain(1)
        sub = [self.entry("." .ljust(11), 0x10, clus, 0),
               self.entry("..".ljust(11), 0x10, parent_clus, 0)]
        parent_entries.append(self.entry(n, 0x10, clus, 0))
        return clus, sub

    def flush_dir(self, clus, entries):
        blob = b"".join(entries)
        limit = SEC_PER_CLUS * SEC
        nclus = max(1, -(-len(blob) // limit))
        if nclus > 1:                       # grow the chain if needed
            c = clus
            for i in range(1, nclus):
                nxt = self.alloc_chain(1)
                self.fat[c] = nxt
                c = nxt
        # write across the chain
        c, pos = clus, 0
        while pos < len(blob):
            off = self.clus_off(c)
            chunk = blob[pos:pos + limit]
            self.img[off:off + len(chunk)] = chunk
            pos += limit
            c = self.fat[c] if self.fat[c] != 0xFFFF else c

    def finish(self, label="SPECCY"):
        # MBR
        mbr = bytearray(SEC)
        mbr[446:462] = struct.pack("<B3sB3sII", 0x00, b"\xFE\xFF\xFF", 0x06,
                                   b"\xFE\xFF\xFF", PART_START, PART_SECS)
        mbr[510:512] = b"\x55\xAA"
        self.img[0:SEC] = mbr

        # Boot sector / BPB
        bs = bytearray(SEC)
        bs[0:3] = b"\xEB\x3C\x90"
        bs[3:11] = b"MSDOS5.0"
        struct.pack_into("<HBHBHHBHHHII", bs, 11,
                         SEC, SEC_PER_CLUS, RESERVED, NFATS, ROOT_ENTRIES,
                         PART_SECS if PART_SECS < 0x10000 else 0,
                         0xF8, FAT_SECS, 63, 16, PART_START,
                         0 if PART_SECS < 0x10000 else PART_SECS)
        bs[38] = 0x29
        struct.pack_into("<I", bs, 39, 0x20260802)
        bs[43:54] = label.ljust(11).encode()
        bs[54:62] = b"FAT16   "
        bs[510:512] = b"\x55\xAA"
        self.img[PART_START * SEC:(PART_START + 1) * SEC] = bs

        # FATs
        fat_blob = b"".join(struct.pack("<H", v) for v in self.fat)
        for i in range(NFATS):
            off = (PART_START + RESERVED + i * FAT_SECS) * SEC
            self.img[off:off + len(fat_blob)] = fat_blob

        # Root directory
        root_off = (PART_START + RESERVED + NFATS * FAT_SECS) * SEC
        blob = b"".join(self.root)
        self.img[root_off:root_off + len(blob)] = blob


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    out = sys.argv[1]
    fs = Fat16()
    dirs = {}                       # DEST dir name -> (clus, entries)

    args = sys.argv[2:]
    i = 0
    while i < len(args):
        if args[i] == "--tree" and i + 1 < len(args):
            tree = args[i + 1]; i += 2
            for name in sorted(os.listdir(tree)):
                p = os.path.join(tree, name)
                if os.path.isfile(p):
                    fs.add_file(fs.root, name, open(p, "rb").read())
                elif os.path.isdir(p):
                    made = fs.add_dir(fs.root, name, 0)
                    if not made:
                        continue
                    clus, sub = made
                    for f in sorted(os.listdir(p)):
                        fp = os.path.join(p, f)
                        if os.path.isfile(fp):
                            fs.add_file(sub, f, open(fp, "rb").read())
                    fs.flush_dir(clus, sub)
                    print(f"  dir {name}: {len(sub)-2} files")
        elif args[i] == "--add" and i + 1 < len(args):
            src, dest = args[i + 1].split("=", 1); i += 2
            data = open(src, "rb").read()
            if "/" in dest:
                d, f = dest.split("/", 1)
                if d not in dirs:
                    made = fs.add_dir(fs.root, d, 0)
                    if made:
                        dirs[d] = (made[0], made[1])
                if d in dirs:
                    fs.add_file(dirs[d][1], f, data)
            else:
                fs.add_file(fs.root, dest, data)
        else:
            sys.exit(f"unknown arg {args[i]}\n{__doc__}")

    for d, (clus, sub) in dirs.items():
        fs.flush_dir(clus, sub)

    fs.finish()
    open(out, "wb").write(fs.img)
    print(f"wrote {out}: 16 MB FAT16, {len(fs.root)} root entries, "
          f"{fs.next_clus - 2} clusters used")


if __name__ == "__main__":
    main()
