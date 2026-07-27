// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vjoystick.h for the primary calling header

#include "Vjoystick__pch.h"

void Vjoystick___024root___ctor_var_reset(Vjoystick___024root* vlSelf);

Vjoystick___024root::Vjoystick___024root(Vjoystick__Syms* symsp, const char* namep)
 {
    vlSymsp = symsp;
    vlNamep = strdup(namep);
    // Reset structure values
    Vjoystick___024root___ctor_var_reset(this);
}

void Vjoystick___024root::__Vconfigure(bool first) {
    (void)first;  // Prevent unused variable warning
}

Vjoystick___024root::~Vjoystick___024root() {
    VL_DO_DANGLING(std::free(const_cast<char*>(vlNamep)), vlNamep);
}
