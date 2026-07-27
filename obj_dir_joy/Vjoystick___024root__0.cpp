// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vjoystick.h for the primary calling header

#include "Vjoystick__pch.h"

bool Vjoystick___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___trigger_anySet__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        if (in[n]) {
            return (1U);
        }
        n = ((IData)(1U) + n);
    } while ((1U > n));
    return (0U);
}

void Vjoystick___024root___trigger_orInto__act_vec_vec(VlUnpacked<QData/*63:0*/, 1> &out, const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___trigger_orInto__act_vec_vec\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        out[n] = (out[n] | in[n]);
        n = ((IData)(1U) + n);
    } while ((0U >= n));
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vjoystick___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG

bool Vjoystick___024root___eval_phase__act(Vjoystick___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___eval_phase__act\n"); );
    Vjoystick__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    {
        // Inlined CFunc: _eval_triggers_vec__act
        vlSelfRef.__VactTriggered[0U] = (QData)((IData)(
                                                        ((IData)(vlSelfRef.clk) 
                                                         & (~ (IData)(vlSelfRef.__Vtrigprevexpr___TOP__clk__0)))));
        vlSelfRef.__Vtrigprevexpr___TOP__clk__0 = vlSelfRef.clk;
    }
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vjoystick___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
    }
#endif
    Vjoystick___024root___trigger_orInto__act_vec_vec(vlSelfRef.__VnbaTriggered, vlSelfRef.__VactTriggered);
    return (0U);
}

void Vjoystick___024root___trigger_clear__act(VlUnpacked<QData/*63:0*/, 1> &out) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___trigger_clear__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        out[n] = 0ULL;
        n = ((IData)(1U) + n);
    } while ((1U > n));
}

extern const VlUnpacked<CData/*7:0*/, 32> Vjoystick__ConstPool__TABLE_hc7e26bc0_0;

bool Vjoystick___024root___eval_phase__nba(Vjoystick___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___eval_phase__nba\n"); );
    Vjoystick__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VnbaExecute;
    // Body
    __VnbaExecute = Vjoystick___024root___trigger_anySet__act(vlSelfRef.__VnbaTriggered);
    if (__VnbaExecute) {
        {
            // Inlined CFunc: _eval_nba
            if ((1ULL & vlSelfRef.__VnbaTriggered[0U])) {
                {
                    // Inlined CFunc: _nba_sequent__TOP__0
                    CData/*4:0*/ __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__state;
                    __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__state = 0;
                    CData/*6:0*/ __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__joystick__DOT__timer;
                    __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__joystick__DOT__timer = 0;
                    __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__joystick__DOT__timer 
                        = vlSelfRef.joystick__DOT__timer;
                    __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__state 
                        = vlSelfRef.state;
                    if (vlSelfRef.rst) {
                        __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__state = 0U;
                        __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__joystick__DOT__timer = 0U;
                    } else if (((IData)(vlSelfRef.joystick__DOT__sync1) 
                                == (IData)(vlSelfRef.state))) {
                        __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__joystick__DOT__timer = 0U;
                    } else if ((0x40U == (IData)(vlSelfRef.joystick__DOT__timer))) {
                        __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__state 
                            = vlSelfRef.joystick__DOT__sync1;
                        __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__joystick__DOT__timer = 0U;
                    } else {
                        __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__joystick__DOT__timer 
                            = (0x0000007fU & ((IData)(1U) 
                                              + (IData)(vlSelfRef.joystick__DOT__timer)));
                    }
                    vlSelfRef.joystick__DOT__timer 
                        = __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__joystick__DOT__timer;
                    vlSelfRef.state = __Vinline_0__eval_nba___Vinline_0__nba_sequent__TOP__0___Vdly__state;
                    vlSelfRef.kempston = Vjoystick__ConstPool__TABLE_hc7e26bc0_0
                        [vlSelfRef.state];
                    vlSelfRef.joystick__DOT__sync1 
                        = vlSelfRef.joystick__DOT__sync0;
                    vlSelfRef.joystick__DOT__sync0 
                        = (0x0000001fU & (~ (IData)(vlSelfRef.pin_n)));
                }
            }
        }
        Vjoystick___024root___trigger_clear__act(vlSelfRef.__VnbaTriggered);
    }
    return (__VnbaExecute);
}

void Vjoystick___024root___eval(Vjoystick___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___eval\n"); );
    Vjoystick__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VnbaIterCount;
    // Body
    __VnbaIterCount = 0U;
    do {
        if (VL_UNLIKELY(((0x00002710U < __VnbaIterCount)))) {
#ifdef VL_DEBUG
            Vjoystick___024root___dump_triggers__act(vlSelfRef.__VnbaTriggered, "nba"s);
#endif
            VL_FATAL_MT("rtl/joystick.v", 33, "", "DIDNOTCONVERGE: NBA region did not converge after '--converge-limit' of 10000 tries");
        }
        __VnbaIterCount = ((IData)(1U) + __VnbaIterCount);
        vlSelfRef.__VactIterCount = 0U;
        do {
            if (VL_UNLIKELY(((0x00002710U < vlSelfRef.__VactIterCount)))) {
#ifdef VL_DEBUG
                Vjoystick___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
#endif
                VL_FATAL_MT("rtl/joystick.v", 33, "", "DIDNOTCONVERGE: Active region did not converge after '--converge-limit' of 10000 tries");
            }
            vlSelfRef.__VactIterCount = ((IData)(1U) 
                                         + vlSelfRef.__VactIterCount);
            vlSelfRef.__VactPhaseResult = Vjoystick___024root___eval_phase__act(vlSelf);
        } while (vlSelfRef.__VactPhaseResult);
        vlSelfRef.__VnbaPhaseResult = Vjoystick___024root___eval_phase__nba(vlSelf);
    } while (vlSelfRef.__VnbaPhaseResult);
}

#ifdef VL_DEBUG
void Vjoystick___024root___eval_debug_assertions(Vjoystick___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___eval_debug_assertions\n"); );
    Vjoystick__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if (VL_UNLIKELY(((vlSelfRef.clk & 0xfeU)))) {
        Verilated::overWidthError("clk");
    }
    if (VL_UNLIKELY(((vlSelfRef.rst & 0xfeU)))) {
        Verilated::overWidthError("rst");
    }
    if (VL_UNLIKELY(((vlSelfRef.pin_n & 0xe0U)))) {
        Verilated::overWidthError("pin_n");
    }
}
#endif  // VL_DEBUG
