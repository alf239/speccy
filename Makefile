VERILATOR ?= verilator

TOP  := speccy_video_top
RTL  := rtl/video_timing.v rtl/vram.v rtl/video.v rtl/palette.v rtl/scandoubler.v rtl/speccy_video_top.v
SIM  := sim/main.cpp
MDIR := obj_dir
EXE  := $(MDIR)/speccy_video_sim

VFLAGS := --cc --exe --build -j 0 \
          --top-module $(TOP) \
          --Mdir $(MDIR) \
          -o speccy_video_sim \
          -Wall -Wno-DECLFILENAME -Wno-PINCONNECTEMPTY

all: $(EXE)

$(EXE): $(RTL) $(SIM)
	$(VERILATOR) $(VFLAGS) $(RTL) $(SIM)

# Dump frames to out/. Pass SCR=path/to/file.scr to use a real screen instead
# of the synthetic test pattern, FRAMES=n to change the count.
# NOTE: FRAMES must not get a global default here -- `boot` below has its own
# fallback, and a global would silently override it (it did: `make boot` ran
# 8 frames and captured the middle of the RAM test instead of the (c) screen).
run: all
	@mkdir -p out
	./$(EXE) --frames $(or $(FRAMES),8) --out out $(if $(SCR),--scr $(SCR),)

open: run
	@open out/frame00.bmp

# Joystick testbench -- built with a short debounce so the test runs quickly.
# DEBOUNCE must match the value in sim/joystick_tb.cpp.
JOY_MDIR := obj_dir_joy
JOY_EXE  := $(JOY_MDIR)/joystick_tb

$(JOY_EXE): rtl/joystick.v sim/joystick_tb.cpp
	$(VERILATOR) --cc --exe --build -j 0 --top-module joystick \
	  --Mdir $(JOY_MDIR) -o joystick_tb \
	  -Wall -Wno-DECLFILENAME \
	  -GDEBOUNCE_CYCLES=64 \
	  rtl/joystick.v sim/joystick_tb.cpp

joytest: $(JOY_EXE)
	./$(JOY_EXE)

# Bus testbench -- the harness pretends to be the Z80.
BUS_MDIR := obj_dir_bus
BUS_EXE  := $(BUS_MDIR)/bus_tb
BUS_RTL  := rtl/video_timing.v rtl/vram.v rtl/ram.v rtl/video.v rtl/palette.v \
            rtl/scandoubler.v rtl/keyboard.v rtl/speccy.v sim/speccy_tb_top.v

# Known ROM contents for the bus tests; must match test_rom_byte() in bus_tb.cpp
sim/test_rom.hex:
	@python3 -c "print('\n'.join('%02X' % ((i*7+3)&0xFF) for i in range(16384)))" > $@

$(BUS_EXE): $(BUS_RTL) sim/bus_tb.cpp sim/test_rom.hex
	$(VERILATOR) --cc --exe --build -j 0 --top-module speccy_tb_top \
	  --Mdir $(BUS_MDIR) -o bus_tb \
	  -Wall -Wno-DECLFILENAME -Wno-PINCONNECTEMPTY \
	  $(BUS_RTL) sim/bus_tb.cpp

bustest: $(BUS_EXE)
	./$(BUS_EXE)

# CPU smoke test -- TV80 executing a generated ROM against the machine.
CPU_MDIR := obj_dir_cpu
CPU_EXE  := $(CPU_MDIR)/cpu_tb
TV80_RTL := rtl/tv80/tv80s.v rtl/tv80/tv80_core.v rtl/tv80/tv80_alu.v \
            rtl/tv80/tv80_mcode.v rtl/tv80/tv80_reg.v
CPU_RTL  := rtl/video_timing.v rtl/vram.v rtl/ram.v rtl/video.v rtl/palette.v \
            rtl/scandoubler.v rtl/keyboard.v rtl/speccy.v rtl/speccy48.v \
            $(TV80_RTL) sim/speccy48_tb_top.v

sim/cpu_rom.hex: sim/make_cpu_rom.py
	python3 sim/make_cpu_rom.py $@

$(CPU_EXE): $(CPU_RTL) sim/cpu_tb.cpp sim/cpu_rom.hex
	$(VERILATOR) --cc --exe --build -j 0 --top-module speccy48_tb_top \
	  --Mdir $(CPU_MDIR) -o cpu_tb \
	  -Wall -Wno-DECLFILENAME -Wno-PINCONNECTEMPTY \
	  tv80.vlt $(CPU_RTL) sim/cpu_tb.cpp

cputest: $(CPU_EXE)
	./$(CPU_EXE)

# Boot test -- the full machine watched like a monitor (sync-locked capture).
# Default ROM is the smoke ROM so this runs without any copyrighted image;
# point it at the real thing with:  make boot ROM=path/to/48.rom FRAMES=175
BOOT_MDIR := obj_dir_boot
BOOT_EXE  := $(BOOT_MDIR)/boot_tb
BOOT_RTL  := rtl/video_timing.v rtl/vram.v rtl/ram.v rtl/video.v rtl/palette.v \
             rtl/ps2_rx.v rtl/ps2_keyboard.v \
             rtl/scandoubler.v rtl/keyboard.v rtl/speccy.v rtl/speccy48.v \
             $(TV80_RTL) sim/boot_tb_top.v
# The ROM path is baked in as out/bootrom.hex; the file is re-generated per
# run ($readmemh reads at runtime, so no rebuild when the ROM changes).
ROM ?=

$(BOOT_EXE): $(BOOT_RTL) sim/boot_tb.cpp
	$(VERILATOR) --cc --exe --build -j 0 --top-module boot_tb_top \
	  --Mdir $(BOOT_MDIR) -o boot_tb \
	  -Wall -Wno-DECLFILENAME -Wno-PINCONNECTEMPTY \
	  -GROM_FILE='"out/bootrom.hex"' \
	  -GVRAM_FILE='"out/snap_vram.hex"' \
	  -GRAM_FILE='"out/snap_ram.hex"' \
	  -GSTUB_FILE='"out/snap_stub.hex"' \
	  tv80.vlt $(BOOT_RTL) sim/boot_tb.cpp

# Placeholder snapshot files so non-snapshot runs stay warning-free.
out/snap_stub.hex:
	@mkdir -p out
	@python3 -c "print('00\n'*256, end='')"   > out/snap_stub.hex
	@python3 -c "print('00\n'*16384, end='')" > out/snap_vram.hex
	@python3 -c "print('00\n'*32768, end='')" > out/snap_ram.hex

boottest: $(BOOT_EXE) sim/cpu_rom.hex out/snap_stub.hex
	@mkdir -p out
	cp sim/cpu_rom.hex out/bootrom.hex
	./$(BOOT_EXE) --frames 8 --expect-smoke --out out/boot_smoke.bmp

boot: $(BOOT_EXE)
	@test -n "$(ROM)" || { echo "usage: make boot ROM=path/to/48.rom [FRAMES=n]"; exit 1; }
	@mkdir -p out
	python3 tools/bin2hex.py $(ROM) out/bootrom.hex
	./$(BOOT_EXE) --frames $(or $(FRAMES),175) --out out/boot.bmp $(if $(TYPE),--type "$(TYPE)",)
	@echo "wrote out/boot.bmp"

# PS/2 receiver + scancode-to-matrix mapper.
PS2_MDIR := obj_dir_ps2
PS2_EXE  := $(PS2_MDIR)/ps2_tb

$(PS2_EXE): rtl/ps2_rx.v rtl/ps2_keyboard.v sim/ps2_tb_top.v sim/ps2_tb.cpp
	$(VERILATOR) --cc --exe --build -j 0 --top-module ps2_tb_top \
	  --Mdir $(PS2_MDIR) -o ps2_tb \
	  -Wall -Wno-DECLFILENAME \
	  rtl/ps2_rx.v rtl/ps2_keyboard.v sim/ps2_tb_top.v sim/ps2_tb.cpp

ps2test: $(PS2_EXE)
	./$(PS2_EXE)

# Snapshot regression: synthetic .z80 through snap2hex, booted via the
# overlay, checked on the virtual monitor. No copyrighted bytes involved.
snaptest: $(BOOT_EXE) sim/cpu_rom.hex
	@mkdir -p out
	python3 tools/make_test_snap.py out/test.z80
	python3 tools/snap2hex.py out/test.z80 out
	cp sim/cpu_rom.hex out/bootrom.hex
	./$(BOOT_EXE) --frames 8 --expect-snap --out out/snap_test.bmp

# Boot a real snapshot:  make snap SNAP=game.z80 ROM=48.rom [FRAMES=n]
snap: $(BOOT_EXE)
	@test -n "$(SNAP)" -a -n "$(ROM)" || { echo "usage: make snap SNAP=game.z80 ROM=48.rom [FRAMES=n]"; exit 1; }
	@mkdir -p out
	python3 tools/snap2hex.py $(SNAP) out
	python3 tools/bin2hex.py $(ROM) out/bootrom.hex
	./$(BOOT_EXE) --snap --frames $(or $(FRAMES),16) --out out/snap.bmp
	@echo "wrote out/snap.bmp"

# Everything that can be checked without hardware.
test: run joytest bustest cputest boottest ps2test snaptest

lint:
	$(VERILATOR) --lint-only --top-module $(TOP) -Wall -Wno-DECLFILENAME -Wno-PINCONNECTEMPTY $(RTL)

clean:
	rm -rf $(MDIR) $(JOY_MDIR) $(BUS_MDIR) $(CPU_MDIR) $(BOOT_MDIR) $(PS2_MDIR) out sim/test_rom.hex sim/cpu_rom.hex

.PHONY: all run open lint clean joytest bustest cputest boottest boot ps2test snaptest snap test
