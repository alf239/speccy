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
FRAMES ?= 8
run: all
	@mkdir -p out
	./$(EXE) --frames $(FRAMES) --out out $(if $(SCR),--scr $(SCR),)

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

# Everything that can be checked without hardware.
test: run joytest bustest

lint:
	$(VERILATOR) --lint-only --top-module $(TOP) -Wall -Wno-DECLFILENAME -Wno-PINCONNECTEMPTY $(RTL)

clean:
	rm -rf $(MDIR) $(JOY_MDIR) $(BUS_MDIR) out sim/test_rom.hex

.PHONY: all run open lint clean joytest bustest test
