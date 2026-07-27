# 50 MHz board oscillator; everything else derives from the PLL.
create_clock -name clk50 -period 20.000 [get_ports {MAX10_CLK1_50}]
derive_pll_clocks
derive_clock_uncertainty

# Human-speed and DAC-bound I/O -- no timing relationship worth constraining
# at 14 MHz internal speed.
set_false_path -from [get_ports {KEY[*] SW[*] GPIO[*]}]
set_false_path -to   [get_ports {LEDR[*] HEX0[*] HEX1[*] HEX2[*] HEX3[*] HEX4[*] HEX5[*] VGA_* GPIO[*]}]
