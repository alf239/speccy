// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vspeccy_tb_top.h for the primary calling header

#include "Vspeccy_tb_top__pch.h"

void Vspeccy_tb_top___024root___ctor_var_reset(Vspeccy_tb_top___024root* vlSelf);

Vspeccy_tb_top___024root::Vspeccy_tb_top___024root(Vspeccy_tb_top__Syms* symsp, const char* namep)
 {
    vlSymsp = symsp;
    vlNamep = strdup(namep);
    // Reset structure values
    Vspeccy_tb_top___024root___ctor_var_reset(this);
}

void Vspeccy_tb_top___024root::__Vconfigure(bool first) {
    (void)first;  // Prevent unused variable warning
}

Vspeccy_tb_top___024root::~Vspeccy_tb_top___024root() {
    VL_DO_DANGLING(std::free(const_cast<char*>(vlNamep)), vlNamep);
}
