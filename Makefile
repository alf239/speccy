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

# Everything that can be checked without hardware.
test: run joytest

lint:
	$(VERILATOR) --lint-only --top-module $(TOP) -Wall -Wno-DECLFILENAME -Wno-PINCONNECTEMPTY $(RTL)

clean:
	rm -rf $(MDIR) $(JOY_MDIR) out

.PHONY: all run open lint clean joytest test
