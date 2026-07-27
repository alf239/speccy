// ---------------------------------------------------------------------------
// pll -- 50 MHz in, 14 MHz out.  50 x 7/25 = 14 exactly.
//
// Quartus only. This is a direct altpll instantiation -- the same thing the
// IP Catalog wizard generates, written by hand so the project carries no
// generated files and the wizard step disappears from the bring-up.
//
// MAX 10 PLL: Fvco = 50 MHz x 7 = 350 MHz, inside the 600-1300... no --
// inside the supported VCO range after the internal post-divider Quartus
// chooses. If Quartus objects to this m/n choice it will say so at synthesis
// and pick its own equivalent; the output frequency constraint is what
// matters.
// ---------------------------------------------------------------------------

module pll (
    input  wire areset,
    input  wire inclk0,
    output wire c0,
    output wire locked
);

    wire [4:0] sub_wire_clk;
    assign c0 = sub_wire_clk[0];

    altpll #(
        .bandwidth_type          ("AUTO"),
        .clk0_divide_by          (25),
        .clk0_duty_cycle         (50),
        .clk0_multiply_by        (7),
        .clk0_phase_shift        ("0"),
        .compensate_clock        ("CLK0"),
        .inclk0_input_frequency  (20000),        // period in ps: 50 MHz
        .intended_device_family  ("MAX 10"),
        .lpm_hint                ("CBX_MODULE_PREFIX=pll"),
        .lpm_type                ("altpll"),
        .operation_mode          ("NORMAL"),
        .pll_type                ("AUTO"),
        .port_activeclock        ("PORT_UNUSED"),
        .port_areset             ("PORT_USED"),
        .port_clkbad0            ("PORT_UNUSED"),
        .port_clkbad1            ("PORT_UNUSED"),
        .port_clkloss            ("PORT_UNUSED"),
        .port_clkswitch          ("PORT_UNUSED"),
        .port_configupdate       ("PORT_UNUSED"),
        .port_fbin               ("PORT_UNUSED"),
        .port_inclk0             ("PORT_USED"),
        .port_inclk1             ("PORT_UNUSED"),
        .port_locked             ("PORT_USED"),
        .port_pfdena             ("PORT_UNUSED"),
        .port_phasecounterselect ("PORT_UNUSED"),
        .port_phasedone          ("PORT_UNUSED"),
        .port_phasestep          ("PORT_UNUSED"),
        .port_phaseupdown        ("PORT_UNUSED"),
        .port_pllena             ("PORT_UNUSED"),
        .port_scanaclr           ("PORT_UNUSED"),
        .port_scanclk            ("PORT_UNUSED"),
        .port_scanclkena         ("PORT_UNUSED"),
        .port_scandata           ("PORT_UNUSED"),
        .port_scandataout        ("PORT_UNUSED"),
        .port_scandone           ("PORT_UNUSED"),
        .port_scanread           ("PORT_UNUSED"),
        .port_scanwrite          ("PORT_UNUSED"),
        .port_clk0               ("PORT_USED"),
        .port_clk1               ("PORT_UNUSED"),
        .port_clk2               ("PORT_UNUSED"),
        .port_clk3               ("PORT_UNUSED"),
        .port_clk4               ("PORT_UNUSED"),
        .port_clk5               ("PORT_UNUSED"),
        .port_clkena0            ("PORT_UNUSED"),
        .port_clkena1            ("PORT_UNUSED"),
        .port_clkena2            ("PORT_UNUSED"),
        .port_clkena3            ("PORT_UNUSED"),
        .port_clkena4            ("PORT_UNUSED"),
        .port_clkena5            ("PORT_UNUSED"),
        .port_extclk0            ("PORT_UNUSED"),
        .port_extclk1            ("PORT_UNUSED"),
        .port_extclk2            ("PORT_UNUSED"),
        .port_extclk3            ("PORT_UNUSED"),
        .self_reset_on_loss_lock ("OFF"),
        .width_clock             (5)
    ) altpll_component (
        .areset       (areset),
        .inclk        ({1'b0, inclk0}),
        .clk          (sub_wire_clk),
        .locked       (locked),
        // unused inputs tied to their documented idle values
        .activeclock  (),
        .clkbad       (),
        .clkena       ({6{1'b1}}),
        .clkloss      (),
        .clkswitch    (1'b0),
        .configupdate (1'b0),
        .enable0      (),
        .enable1      (),
        .extclk       (),
        .extclkena    ({4{1'b1}}),
        .fbin         (1'b1),
        .fbmimicbidir (),
        .fbout        (),
        .fref         (),
        .icdrclk      (),
        .pfdena       (1'b1),
        .phasecounterselect ({4{1'b1}}),
        .phasedone    (),
        .phasestep    (1'b1),
        .phaseupdown  (1'b1),
        .pllena       (1'b1),
        .scanaclr     (1'b0),
        .scanclk      (1'b0),
        .scanclkena   (1'b1),
        .scandata     (1'b0),
        .scandataout  (),
        .scandone     (),
        .scanread     (1'b0),
        .scanwrite    (1'b0),
        .sclkout0     (),
        .sclkout1     (),
        .vcooverrange (),
        .vcounderrange()
    );

endmodule
