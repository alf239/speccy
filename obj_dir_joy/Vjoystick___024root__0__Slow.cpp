// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vjoystick.h for the primary calling header

#include "Vjoystick__pch.h"

VL_ATTR_COLD void Vjoystick___024root___eval_static(Vjoystick___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___eval_static\n"); );
    Vjoystick__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__Vtrigprevexpr___TOP__clk__0 = vlSelfRef.clk;
}

VL_ATTR_COLD void Vjoystick___024root___eval_initial(Vjoystick___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___eval_initial\n"); );
    Vjoystick__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
}

VL_ATTR_COLD void Vjoystick___024root___eval_final(Vjoystick___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___eval_final\n"); );
    Vjoystick__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vjoystick___024root___dump_triggers__stl(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG
VL_ATTR_COLD bool Vjoystick___024root___eval_phase__stl(Vjoystick___024root* vlSelf);

VL_ATTR_COLD void Vjoystick___024root___eval_settle(Vjoystick___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___eval_settle\n"); );
    Vjoystick__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VstlIterCount;
    // Body
    __VstlIterCount = 0U;
    vlSelfRef.__VstlFirstIteration = 1U;
    do {
        if (VL_UNLIKELY(((0x00002710U < __VstlIterCount)))) {
#ifdef VL_DEBUG
            Vjoystick___024root___dump_triggers__stl(vlSelfRef.__VstlTriggered, "stl"s);
#endif
            VL_FATAL_MT("rtl/joystick.v", 33, "", "DIDNOTCONVERGE: Settle region did not converge after '--converge-limit' of 10000 tries");
        }
        __VstlIterCount = ((IData)(1U) + __VstlIterCount);
        vlSelfRef.__VstlPhaseResult = Vjoystick___024root___eval_phase__stl(vlSelf);
        vlSelfRef.__VstlFirstIteration = 0U;
    } while (vlSelfRef.__VstlPhaseResult);
}

VL_ATTR_COLD bool Vjoystick___024root___trigger_anySet__stl(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vjoystick___024root___dump_triggers__stl(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___dump_triggers__stl\n"); );
    // Body
    if ((1U & (~ (IData)(Vjoystick___024root___trigger_anySet__stl(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: Internal 'stl' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD bool Vjoystick___024root___trigger_anySet__stl(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___trigger_anySet__stl\n"); );
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

extern const VlUnpacked<CData/*7:0*/, 32> Vjoystick__ConstPool__TABLE_hc7e26bc0_0;

VL_ATTR_COLD bool Vjoystick___024root___eval_phase__stl(Vjoystick___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___eval_phase__stl\n"); );
    Vjoystick__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VstlExecute;
    // Body
    {
        // Inlined CFunc: _eval_triggers_vec__stl
        vlSelfRef.__VstlTriggered[0U] = ((0xfffffffffffffffeULL 
                                          & vlSelfRef.__VstlTriggered[0U]) 
                                         | (IData)((IData)(vlSelfRef.__VstlFirstIteration)));
    }
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vjoystick___024root___dump_triggers__stl(vlSelfRef.__VstlTriggered, "stl"s);
    }
#endif
    __VstlExecute = Vjoystick___024root___trigger_anySet__stl(vlSelfRef.__VstlTriggered);
    if (__VstlExecute) {
        {
            // Inlined CFunc: _eval_stl
            if ((1ULL & vlSelfRef.__VstlTriggered[0U])) {
                {
                    // Inlined CFunc: _stl_sequent__TOP__0
                    vlSelfRef.kempston = Vjoystick__ConstPool__TABLE_hc7e26bc0_0
                        [vlSelfRef.state];
                }
            }
        }
    }
    return (__VstlExecute);
}

bool Vjoystick___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vjoystick___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___dump_triggers__act\n"); );
    // Body
    if ((1U & (~ (IData)(Vjoystick___024root___trigger_anySet__act(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: @(posedge clk)\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vjoystick___024root___ctor_var_reset(Vjoystick___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vjoystick___024root___ctor_var_reset\n"); );
    Vjoystick__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    const uint64_t __VscopeHash = VL_MURMUR64_HASH(vlSelf->vlNamep);
    vlSelf->clk = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 16707436170211756652ull);
    vlSelf->rst = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 18209466448985614591ull);
    vlSelf->pin_n = VL_SCOPED_RAND_RESET_I(5, __VscopeHash, 7165702545783279652ull);
    vlSelf->state = VL_SCOPED_RAND_RESET_I(5, __VscopeHash, 9404372463396948974ull);
    vlSelf->kempston = VL_SCOPED_RAND_RESET_I(8, __VscopeHash, 8741315273541376864ull);
    vlSelf->joystick__DOT__sync0 = VL_SCOPED_RAND_RESET_I(5, __VscopeHash, 17047698264025547710ull);
    vlSelf->joystick__DOT__sync1 = VL_SCOPED_RAND_RESET_I(5, __VscopeHash, 6640187618480500148ull);
    vlSelf->joystick__DOT__timer = VL_SCOPED_RAND_RESET_I(7, __VscopeHash, 8050789660078929866ull);
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VstlTriggered[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VactTriggered[__Vi0] = 0;
    }
    vlSelf->__Vtrigprevexpr___TOP__clk__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VnbaTriggered[__Vi0] = 0;
    }
}
