#!/usr/bin/env python3
"""Generate the PCB floorplan sketch and the 1:1 print template from the
measurement table below. The table is the single source of truth; when the
calipers speak, edit here and rerun:

    python3 tools/gen_pcb_sketch.py

Frame: component side facing you, ports at top, keyboard edge at bottom.
X from the LEFT edge, Y from the BOTTOM edge (converted internally).
"""

# ---- MEASURED (the law) ---------------------------------------------------
W, H = 213.5, 130.0                 # board outline, mm

HOLES = [
    # (x, y_from_bottom, dia, label, legacy?)
    (5.0,        H-44.5,  4.0, "4.0 side-L (5 fr left, 44.5 fr top)", False),
    (W-5.0,      H-44.5,  4.0, "4.0 side-R (5 fr right, 44.5 fr top)", False),
    (4.75,       4.75,    4.0, "4.0 BL (~4.75 fr both)", False),
    (W-4.75,     4.75,    4.0, "4.0 BR (~4.75 fr both)", False),
    (W/2,        40.0,    2.0, "2.0 centre mount (X assumed centred)", False),
    (W/2,        H-5.0,   3.0, "3.0 case bolt pass (X assumed centred)", False),
    (12.0,       H-6.0,   3.5, "legacy, unused", True),
    (W-14.0,     H-6.5,   3.5, "legacy, unused", True),
]

SPEAKER = (W-21.5, 12.5, 10.0)      # centre x photo-est; y moved 6 down per print test

KB1 = dict(x_anchor=30.0,   y=56.0, pins=5, direction=+1,
           label="KB1 5-way, contacts UP, leftmost contact 30.0 fr left")
KB2 = dict(x_anchor=W-27.0, y=64.0, pins=8, direction=-1,
           label="KB2 8-way, contacts DOWN, rightmost contact 27.0 fr right")
PITCH = 2.54                        # tiebreak pending: 2.54 vs 2.50

# ---- generation -----------------------------------------------------------
S = 4  # px/mm in the floorplan
OX, OY = 70, 90

def px(x): return OX + x*S
def py(y_from_bottom): return OY + (H - y_from_bottom)*S

def slot_geom(k):
    xs = [k["x_anchor"] + k["direction"]*i*PITCH for i in range(k["pins"])]
    return min(xs), max(xs), xs

def gen_template(path):
    parts = []
    for x, y, d, label, legacy in HOLES:
        cls = "legacy" if legacy else "real"
        r = d/2
        parts.append(f'''  <g class="{cls}">
    <circle cx="{x:.2f}" cy="{H-y:.2f}" r="{r}" />
    <line x1="{x-r-2:.2f}" y1="{H-y:.2f}" x2="{x+r+2:.2f}" y2="{H-y:.2f}" />
    <line x1="{x:.2f}" y1="{H-y-r-2:.2f}" x2="{x:.2f}" y2="{H-y+r+2:.2f}" />
    <text x="{x+r+2.5:.2f}" y="{H-y+1.2:.2f}">{label}</text>
  </g>''')
    sx, sy, sd = SPEAKER
    parts.append(f'''  <g class="real">
    <circle cx="{sx:.2f}" cy="{H-sy:.2f}" r="{sd/2}" />
    <line x1="{sx-sd/2-2:.2f}" y1="{H-sy:.2f}" x2="{sx+sd/2+2:.2f}" y2="{H-sy:.2f}" />
    <line x1="{sx:.2f}" y1="{H-sy-sd/2-2:.2f}" x2="{sx:.2f}" y2="{H-sy+sd/2+2:.2f}" />
    <text x="{sx-45:.2f}" y="{H-sy-8:.2f}">speaker hole ~{sd} dia PHOTO-EST: confirm</text>
  </g>''')
    for k in (KB1, KB2):
        lo, hi, xs = slot_geom(k)
        y = H - k["y"]
        ticks = ''.join(f'<line x1="{x:.2f}" y1="{y-4:.2f}" x2="{x:.2f}" y2="{y+4:.2f}" />' for x in xs)
        parts.append(f'''  <g class="slot">
    <rect x="{lo-3:.2f}" y="{y-2.5:.2f}" width="{hi-lo+6:.2f}" height="5" />
    {ticks}
    <text x="{lo-3:.2f}" y="{y+9:.2f}">{k["label"]}</text>
  </g>''')
    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="{W}mm" height="{H}mm" viewBox="0 0 {W} {H}" font-family="Helvetica,Arial,sans-serif">
  <style>
    .real circle {{ fill:none; stroke:#000; stroke-width:0.3; }}
    .real line   {{ stroke:#000; stroke-width:0.15; }}
    .real text   {{ font-size:2.6px; fill:#000; }}
    .legacy circle {{ fill:none; stroke:#888; stroke-width:0.3; stroke-dasharray:1 1; }}
    .legacy line {{ stroke:#888; stroke-width:0.15; }}
    .legacy text {{ font-size:2.6px; fill:#888; }}
    .slot rect   {{ fill:none; stroke:#a00; stroke-width:0.3; }}
    .slot line   {{ stroke:#a00; stroke-width:0.15; }}
    .slot text   {{ font-size:2.6px; fill:#a00; }}
    .edge        {{ fill:none; stroke:#000; stroke-width:0.4; }}
    .meta        {{ font-size:3.2px; fill:#000; }}
    .bar         {{ stroke:#000; stroke-width:0.5; }}
  </style>
  <rect x="0" y="0" width="{W}" height="{H}" class="edge"/>
{chr(10).join(parts)}
  <text x="60" y="26" class="meta">speccy board 1:1 template — outline {W} x {H}. PRINT AT 100%,</text>
  <text x="60" y="31" class="meta">verify the 50 mm bar with calipers before trusting anything.</text>
  <line x1="60" y1="36" x2="110" y2="36" class="bar"/>
  <line x1="60" y1="34" x2="60" y2="38" class="bar"/>
  <line x1="110" y1="34" x2="110" y2="38" class="bar"/>
  <text x="112" y="37.5" class="meta">50 mm</text>
  <text x="60" y="106" class="meta">ports edge = top. Slot ticks at {PITCH} mm pitch: socket pins drifting off the</text>
  <text x="60" y="111" class="meta">ticks over 8 pins means metric 2.50 (caliper span settles it).</text>
</svg>'''
    open(path, 'w').write(svg)

def gen_floorplan(path):
    def blk(x, y_fb, w, h, cls, lines, tx=None, ty=None):
        r = f'  <rect x="{px(x):.0f}" y="{py(y_fb):.0f}" width="{w*S:.0f}" height="{h*S:.0f}" class="{cls}"/>\n'
        tx = px(x)+8 if tx is None else tx
        ty0 = py(y_fb)+20 if ty is None else ty
        for i, ln in enumerate(lines):
            r += f'  <text x="{tx:.0f}" y="{ty0+16*i:.0f}" class="lbl{"2" if cls=="conn" else ""}">{ln}</text>\n'
        return r
    parts = []
    # rear edge
    parts.append(blk(17.5, H-0.5, 30, 12, "conn", ["2x 3.5mm jacks", "line-out + EAR-in"]))
    # rear strip parts around the top bolt-pass at W/2 (keep 103-110.5 clear)
    parts.append(blk(55, H-0.5, 47, 14, "conn", ["rear strip A:", "VGA | microSD"]))
    parts.append(blk(111.5, H-0.5, 37, 14, "conn", ["rear strip B:", "2x DE-9 | JTAG"]))
    parts.append(blk(W-23.5, H-0.5, 20, 12, "conn", ["DC 5-12V", "barrel"]))
    parts.append(blk(W-40, H-15.5, 37, 18, "blk", ["POWER: wide-in buck", "3.3V/2A + protection", "(1982-PSU-proof)"]))
    parts.append(blk(W-63, H-15.5, 21, 18, "blk", ["DIP-4 modes", "RESET + NMI", "case-btn hdr"]))
    parts.append(blk(1.5, H-15.5, 32, 20, "quiet", ["analog corner:", "audio RC, line drv,", "EAR divider/schmitt"]))
    # core corridor
    parts.append(blk(72, 53, 25, 25, "blk", ["10M50SAE144", "22x22 EQFP"]))
    parts.append(blk(100, 63, 23, 11, "blk", ["SDRAM TSOP-54"]))
    parts.append(blk(56, 66, 14, 8, "blk", ["50MHz"]))  # was on the centre mount screw -- paper test catch
    parts.append(blk(80, 92.5, 35, 10, "blk", ["VGA DAC ladder"]))
    parts.append(blk(140, 31, 42, 9, "blk", ["Sinclair joy taps -> rear DE-9"]))
    parts.append(blk(8, 24, 55, 20, "quiet", ["silkscreen: build log, (c) 2026"]))
    parts.append(blk(72, 18, 50, 9, "quiet", ["decoupling farm everywhere"]))
    # speaker + driver
    sx, sy, sd = SPEAKER
    parts.append(f'  <circle cx="{px(sx):.0f}" cy="{py(sy):.0f}" r="{sd/2*S:.0f}" class="hole"/>\n'
                 f'  <text x="{px(sx)-140:.0f}" y="{py(sy)-26:.0f}" class="lbl">speaker hole ~{sd}mm + driver PHOTO-EST</text>\n')
    # KB slots
    for k in (KB1, KB2):
        lo, hi, xs = slot_geom(k)
        anchor = k["x_anchor"]
        parts.append(f'  <rect x="{px(lo-3):.0f}" y="{py(k["y"])-10:.0f}" width="{(hi-lo+6)*S:.0f}" height="20" class="fix"/>\n'
                     f'  <line x1="{px(anchor):.0f}" y1="{py(k["y"])-14:.0f}" x2="{px(anchor):.0f}" y2="{py(k["y"])+14:.0f}" class="cross"/>\n'
                     f'  <rect x="{px(lo-6):.0f}" y="{py(k["y"])-24:.0f}" width="{(hi-lo+12)*S:.0f}" height="52" class="keep"/>\n')
    parts.append(f'  <text x="{px(20):.0f}" y="{py(KB1["y"])+34:.0f}" class="lblr">KB1 5-way (contacts UP): leftmost contact {KB1["x_anchor"]:g} fr left, {KB1["y"]:g} fr bottom</text>\n')
    parts.append(f'  <text x="{px(120):.0f}" y="{py(KB2["y"])-30:.0f}" class="lblr">KB2 8-way (contacts DOWN): rightmost {W-KB2["x_anchor"]:g} fr right, {KB2["y"]:g} fr bottom</text>\n')
    # holes
    for x, y, d, label, legacy in HOLES:
        cls = "keep" if legacy else "hole"
        parts.append(f'  <circle cx="{px(x):.0f}" cy="{py(y):.0f}" r="{max(4, d/2*S):.0f}" class="{cls}"/>\n')
        if not legacy:
            parts.append(f'  <circle cx="{px(x):.0f}" cy="{py(y):.0f}" r="{d/2*S+10:.0f}" class="keep"/>\n')
    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1000 700" font-family="Helvetica,Arial,sans-serif">
  <defs><style>
      .board {{ fill:#0b3d0b; stroke:#333; stroke-width:2; }}
      .blk   {{ fill:#1e6f1e; stroke:#cfe8cf; stroke-width:1.5; }}
      .conn  {{ fill:#8a6d1a; stroke:#ffe08a; stroke-width:1.5; }}
      .fix   {{ fill:#7a1f1f; stroke:#ff9d9d; stroke-width:2.5; }}
      .keep  {{ fill:none; stroke:#ff9d9d; stroke-width:1; stroke-dasharray:4 4; }}
      .quiet {{ fill:#14501e; stroke:#9fd7a9; stroke-width:1; stroke-dasharray:5 4; }}
      .hole  {{ fill:#ddd; stroke:#666; stroke-width:1.5; }}
      .lbl   {{ fill:#f0f0f0; font-size:13px; }}
      .lbl2  {{ fill:#ffe9a8; font-size:12px; }}
      .lblr  {{ fill:#ffc9c9; font-size:12px; }}
      .dim   {{ fill:#bbb; font-size:12px; }}
      .title {{ fill:#222; font-size:17px; font-weight:bold; }}
      .note  {{ fill:#555; font-size:12px; }}
      .cross {{ stroke:#fff; stroke-width:1.5; }}
  </style></defs>
  <rect x="0" y="0" width="1000" height="700" fill="#f4f1e8"/>
  <text x="28" y="30" class="title">speccy board — floorplan (generated; outline {W} x {H} mm)</text>
  <text x="28" y="50" class="note">4 px/mm. Component side facing you, ports at top. Regenerate: python3 tools/gen_pcb_sketch.py</text>
  <rect x="{OX}" y="{OY}" width="{W*S:.0f}" height="{H*S:.0f}" rx="6" class="board"/>
  <text x="420" y="{OY+H*S+24:.0f}" class="dim">{W} mm (measured)</text>
  <text x="16" y="380" class="dim" transform="rotate(-90 26 380)">{H} mm (measured)</text>
{''.join(parts)}
  <text x="{OX}" y="676" class="note">Red = fixed (measured). Dashed = keep-out / legacy. Speaker hole and centre-pair X photo/assumed: confirm. Pitch {PITCH} pending tiebreak.</text>
</svg>'''
    open(path, 'w').write(svg)

if __name__ == "__main__":
    gen_template("pcb/template-1to1.svg")
    gen_floorplan("docs/img/pcb-floorplan.svg")
    print(f"generated for outline {W} x {H}")
