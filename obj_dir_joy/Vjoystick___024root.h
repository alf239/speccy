// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vjoystick.h for the primary calling header

#ifndef VERILATED_VJOYSTICK___024ROOT_H_
#define VERILATED_VJOYSTICK___024ROOT_H_  // guard

#include "verilated.h"


class Vjoystick__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vjoystick___024root final {
  public:

    // DESIGN SPECIFIC STATE
    VL_IN8(clk,0,0);
    VL_IN8(rst,0,0);
    VL_IN8(pin_n,4,0);
    VL_OUT8(state,4,0);
    VL_OUT8(kempston,7,0);
    CData/*4:0*/ joystick__DOT__sync0;
    CData/*4:0*/ joystick__DOT__sync1;
    CData/*6:0*/ joystick__DOT__timer;
    CData/*0:0*/ __VstlFirstIteration;
    CData/*0:0*/ __VstlPhaseResult;
    CData/*0:0*/ __Vtrigprevexpr___TOP__clk__0;
    CData/*0:0*/ __VactPhaseResult;
    CData/*0:0*/ __VnbaPhaseResult;
    IData/*31:0*/ __VactIterCount;
    VlUnpacked<QData/*63:0*/, 1> __VstlTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VactTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VnbaTriggered;

    // INTERNAL VARIABLES
    Vjoystick__Syms* vlSymsp;
    const char* vlNamep;

    // CONSTRUCTORS
    Vjoystick___024root(Vjoystick__Syms* symsp, const char* namep);
    ~Vjoystick___024root();
    VL_UNCOPYABLE(Vjoystick___024root);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
