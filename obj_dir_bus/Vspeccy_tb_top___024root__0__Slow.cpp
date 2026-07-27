// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vspeccy_tb_top.h for the primary calling header

#include "Vspeccy_tb_top__pch.h"

VL_ATTR_COLD void Vspeccy_tb_top___024root___eval_static(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___eval_static\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__Vtrigprevexpr___TOP__clk__0 = vlSelfRef.clk;
    vlSelfRef.__Vtrigprevexpr___TOP__rst__0 = vlSelfRef.rst;
    vlSelfRef.__Vtrigprevexpr___TOP__cpu_a__0 = vlSelfRef.cpu_a;
    vlSelfRef.__Vtrigprevexpr___TOP__cpu_do__0 = vlSelfRef.cpu_do;
    vlSelfRef.__Vtrigprevexpr___TOP__mreq_n__0 = vlSelfRef.mreq_n;
    vlSelfRef.__Vtrigprevexpr___TOP__iorq_n__0 = vlSelfRef.iorq_n;
    vlSelfRef.__Vtrigprevexpr___TOP__rd_n__0 = vlSelfRef.rd_n;
    vlSelfRef.__Vtrigprevexpr___TOP__wr_n__0 = vlSelfRef.wr_n;
    vlSelfRef.__Vtrigprevexpr___TOP__m1_n__0 = vlSelfRef.m1_n;
    vlSelfRef.__Vtrigprevexpr___TOP__key_matrix__0 
        = vlSelfRef.key_matrix;
    vlSelfRef.__Vtrigprevexpr___TOP__joy_state__0 = vlSelfRef.joy_state;
    vlSelfRef.__Vtrigprevexpr___TOP__ear_in__0 = vlSelfRef.ear_in;
    vlSelfRef.__Vtrigprevexpr___TOP__clk__1 = vlSelfRef.clk;
}

VL_ATTR_COLD void Vspeccy_tb_top___024root___eval_initial(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___eval_initial\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    {
        // Inlined CFunc: _eval_initial__TOP
        VL_READMEM_N(true, 8, 16384, 0, "sim/test_rom.hex"s
                     ,  &(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_rom__DOT__mem)
                     , 0, ~0ULL);
    }
}

VL_ATTR_COLD void Vspeccy_tb_top___024root___eval_final(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___eval_final\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vspeccy_tb_top___024root___dump_triggers__stl(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG
VL_ATTR_COLD bool Vspeccy_tb_top___024root___eval_phase__stl(Vspeccy_tb_top___024root* vlSelf);

VL_ATTR_COLD void Vspeccy_tb_top___024root___eval_settle(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___eval_settle\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VstlIterCount;
    // Body
    __VstlIterCount = 0U;
    vlSelfRef.__VstlFirstIteration = 1U;
    do {
        if (VL_UNLIKELY(((0x00002710U < __VstlIterCount)))) {
#ifdef VL_DEBUG
            Vspeccy_tb_top___024root___dump_triggers__stl(vlSelfRef.__VstlTriggered, "stl"s);
#endif
            VL_FATAL_MT("sim/speccy_tb_top.v", 6, "", "DIDNOTCONVERGE: Settle region did not converge after '--converge-limit' of 10000 tries");
        }
        __VstlIterCount = ((IData)(1U) + __VstlIterCount);
        vlSelfRef.__VstlPhaseResult = Vspeccy_tb_top___024root___eval_phase__stl(vlSelf);
        vlSelfRef.__VstlFirstIteration = 0U;
    } while (vlSelfRef.__VstlPhaseResult);
}

VL_ATTR_COLD bool Vspeccy_tb_top___024root___trigger_anySet__stl(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vspeccy_tb_top___024root___dump_triggers__stl(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___dump_triggers__stl\n"); );
    // Body
    if ((1U & (~ (IData)(Vspeccy_tb_top___024root___trigger_anySet__stl(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: Internal 'stl' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD bool Vspeccy_tb_top___024root___trigger_anySet__stl(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___trigger_anySet__stl\n"); );
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

extern const VlUnpacked<CData/*7:0*/, 32> Vspeccy_tb_top__ConstPool__TABLE_hc7e26bc0_0;

VL_ATTR_COLD void Vspeccy_tb_top___024root___stl_sequent__TOP__0(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___stl_sequent__TOP__0\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    SData/*8:0*/ speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__fh;
    speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__fh = 0;
    SData/*8:0*/ speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__fv;
    speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__fv = 0;
    // Body
    vlSelfRef.ce_cpu = (3U == (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__phase));
    vlSelfRef.ce_pix = (1U & (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__phase));
    vlSelfRef.int_n = (0U == (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__int_ctr));
    vlSelfRef.border = vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__border_r;
    if ((0x01c0U <= (0x000003ffU & ((IData)(8U) + (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc))))) {
        speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__fh 
            = (0x000001ffU & ((IData)(0x0048U) + (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc)));
        speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__fv 
            = (0x000001ffU & (((IData)(1U) + (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc)) 
                              & (- (IData)((0x0137U 
                                            != (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc))))));
    } else {
        speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__fh 
            = (0x000001ffU & ((IData)(8U) + (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc)));
        speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__fv 
            = (0x000001ffU & (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc));
    }
    vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__mem_wr 
        = (1U & ((~ (IData)(vlSelfRef.mreq_n)) & (~ (IData)(vlSelfRef.wr_n))));
    vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__io_wr 
        = ((~ (IData)(vlSelfRef.iorq_n)) & ((~ (IData)(vlSelfRef.wr_n)) 
                                            & (IData)(vlSelfRef.m1_n)));
    vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed = 0U;
    if ((1U & (~ ((IData)(vlSelfRef.cpu_a) >> 8U)))) {
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
            = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                              | (IData)(vlSelfRef.key_matrix)));
    }
    if ((1U & (~ ((IData)(vlSelfRef.cpu_a) >> 9U)))) {
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
            = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                              | (IData)((vlSelfRef.key_matrix 
                                         >> 5U))));
    }
    if ((1U & (~ ((IData)(vlSelfRef.cpu_a) >> 0x0000000aU)))) {
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
            = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                              | (IData)((vlSelfRef.key_matrix 
                                         >> 0x0aU))));
    }
    if ((1U & (~ ((IData)(vlSelfRef.cpu_a) >> 0x0000000bU)))) {
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
            = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                              | (IData)((vlSelfRef.key_matrix 
                                         >> 0x0fU))));
    }
    if ((1U & (~ ((IData)(vlSelfRef.cpu_a) >> 0x0000000cU)))) {
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
            = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                              | (IData)((vlSelfRef.key_matrix 
                                         >> 0x14U))));
    }
    if ((1U & (~ ((IData)(vlSelfRef.cpu_a) >> 0x0000000dU)))) {
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
            = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                              | (IData)((vlSelfRef.key_matrix 
                                         >> 0x19U))));
    }
    if ((1U & (~ ((IData)(vlSelfRef.cpu_a) >> 0x0000000eU)))) {
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
            = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                              | (IData)((vlSelfRef.key_matrix 
                                         >> 0x1eU))));
    }
    if ((1U & (~ ((IData)(vlSelfRef.cpu_a) >> 0x0000000fU)))) {
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
            = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                              | (IData)((vlSelfRef.key_matrix 
                                         >> 0x23U))));
    }
    vlSelfRef.cpu_di = (0x000000ffU & ((1U & ((~ (IData)(vlSelfRef.mreq_n)) 
                                              & (~ (IData)(vlSelfRef.rd_n))))
                                        ? ((0U == (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__rd_bank))
                                            ? (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__rom_q)
                                            : ((1U 
                                                == (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__rd_bank))
                                                ? (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__vram_q)
                                                : (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__ramhi_q)))
                                        : ((- (IData)(
                                                      (1U 
                                                       & (~ 
                                                          ((~ (IData)(vlSelfRef.iorq_n)) 
                                                           & ((~ (IData)(vlSelfRef.rd_n)) 
                                                              & (IData)(vlSelfRef.m1_n))))))) 
                                           | ((1U & (IData)(vlSelfRef.cpu_a))
                                               ? (Vspeccy_tb_top__ConstPool__TABLE_hc7e26bc0_0
                                                  [vlSelfRef.joy_state] 
                                                  | (- (IData)(
                                                               (1U 
                                                                & ((IData)(vlSelfRef.cpu_a) 
                                                                   >> 5U)))))
                                               : (0x000000a0U 
                                                  | (((IData)(vlSelfRef.ear_in) 
                                                      << 6U) 
                                                     | (0x0000001fU 
                                                        & (~ (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed)))))))));
}

VL_ATTR_COLD bool Vspeccy_tb_top___024root___eval_phase__stl(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___eval_phase__stl\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
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
        Vspeccy_tb_top___024root___dump_triggers__stl(vlSelfRef.__VstlTriggered, "stl"s);
    }
#endif
    __VstlExecute = Vspeccy_tb_top___024root___trigger_anySet__stl(vlSelfRef.__VstlTriggered);
    if (__VstlExecute) {
        {
            // Inlined CFunc: _eval_stl
            if ((1ULL & vlSelfRef.__VstlTriggered[0U])) {
                Vspeccy_tb_top___024root___stl_sequent__TOP__0(vlSelf);
            }
        }
    }
    return (__VstlExecute);
}

bool Vspeccy_tb_top___024root___trigger_anySet__ico(const VlUnpacked<QData/*63:0*/, 2> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vspeccy_tb_top___024root___dump_triggers__ico(const VlUnpacked<QData/*63:0*/, 2> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___dump_triggers__ico\n"); );
    // Body
    if ((1U & (~ (IData)(Vspeccy_tb_top___024root___trigger_anySet__ico(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: @( clk)\n");
    }
    if ((1U & (IData)((triggers[0U] >> 1U)))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 1 is active: @( rst)\n");
    }
    if ((1U & (IData)((triggers[0U] >> 2U)))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 2 is active: @( cpu_a)\n");
    }
    if ((1U & (IData)((triggers[0U] >> 3U)))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 3 is active: @( cpu_do)\n");
    }
    if ((1U & (IData)((triggers[0U] >> 4U)))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 4 is active: @( mreq_n)\n");
    }
    if ((1U & (IData)((triggers[0U] >> 5U)))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 5 is active: @( iorq_n)\n");
    }
    if ((1U & (IData)((triggers[0U] >> 6U)))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 6 is active: @( rd_n)\n");
    }
    if ((1U & (IData)((triggers[0U] >> 7U)))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 7 is active: @( wr_n)\n");
    }
    if ((1U & (IData)((triggers[0U] >> 8U)))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 8 is active: @( m1_n)\n");
    }
    if ((1U & (IData)((triggers[0U] >> 9U)))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 9 is active: @( key_matrix)\n");
    }
    if ((1U & (IData)((triggers[0U] >> 0x0000000aU)))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 10 is active: @( joy_state)\n");
    }
    if ((1U & (IData)((triggers[0U] >> 0x0000000bU)))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 11 is active: @( ear_in)\n");
    }
    if ((1U & (IData)(triggers[1U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 64 is active: Internal 'ico' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

bool Vspeccy_tb_top___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vspeccy_tb_top___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___dump_triggers__act\n"); );
    // Body
    if ((1U & (~ (IData)(Vspeccy_tb_top___024root___trigger_anySet__act(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: @(posedge clk)\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vspeccy_tb_top___024root___ctor_var_reset(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___ctor_var_reset\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    const uint64_t __VscopeHash = VL_MURMUR64_HASH(vlSelf->vlNamep);
    vlSelf->clk = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 16707436170211756652ull);
    vlSelf->rst = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 18209466448985614591ull);
    vlSelf->ce_cpu = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 3478278084198508283ull);
    vlSelf->ce_pix = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 5281599081232318353ull);
    vlSelf->cpu_a = VL_SCOPED_RAND_RESET_I(16, __VscopeHash, 10193023104960861249ull);
    vlSelf->cpu_do = VL_SCOPED_RAND_RESET_I(8, __VscopeHash, 8603329655250186673ull);
    vlSelf->cpu_di = VL_SCOPED_RAND_RESET_I(8, __VscopeHash, 761664683297855482ull);
    vlSelf->mreq_n = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 17011450694056663149ull);
    vlSelf->iorq_n = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 11899967842544616261ull);
    vlSelf->rd_n = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 4917808325942757789ull);
    vlSelf->wr_n = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 9096275881558974566ull);
    vlSelf->m1_n = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 3174253759424008206ull);
    vlSelf->int_n = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 13998616461741723212ull);
    vlSelf->key_matrix = VL_SCOPED_RAND_RESET_Q(40, __VscopeHash, 17122296032173120298ull);
    vlSelf->joy_state = VL_SCOPED_RAND_RESET_I(5, __VscopeHash, 11843798232013117246ull);
    vlSelf->ear_in = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 13028191974909355171ull);
    vlSelf->speaker = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 8276233359746243045ull);
    vlSelf->mic = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 2719791455349908433ull);
    vlSelf->border = VL_SCOPED_RAND_RESET_I(3, __VscopeHash, 15656004685298392866ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__phase = VL_SCOPED_RAND_RESET_I(2, __VscopeHash, 3177612948532909639ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__mem_wr = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 1800253496687356434ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__rom_q = VL_SCOPED_RAND_RESET_I(8, __VscopeHash, 8032680490691159607ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__vram_q = VL_SCOPED_RAND_RESET_I(8, __VscopeHash, 7390528113905479466ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__ramhi_q = VL_SCOPED_RAND_RESET_I(8, __VscopeHash, 13449981611397175258ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__rd_bank = VL_SCOPED_RAND_RESET_I(2, __VscopeHash, 6483501418207640846ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__io_wr = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 691888720255787063ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__border_r = VL_SCOPED_RAND_RESET_I(3, __VscopeHash, 12503856576922511671ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__px_h = VL_SCOPED_RAND_RESET_I(9, __VscopeHash, 16456249960784875892ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__px_v = VL_SCOPED_RAND_RESET_I(9, __VscopeHash, 2780352775820757860ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__int_ctr = VL_SCOPED_RAND_RESET_I(8, __VscopeHash, 10760048955941146320ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc = VL_SCOPED_RAND_RESET_I(9, __VscopeHash, 4653022218888016414ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc = VL_SCOPED_RAND_RESET_I(9, __VscopeHash, 14874631868133760772ull);
    vlSelf->speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed = VL_SCOPED_RAND_RESET_I(5, __VscopeHash, 4321836025550598685ull);
    for (int __Vi0 = 0; __Vi0 < 32768; ++__Vi0) {
        vlSelf->speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem[__Vi0] = VL_SCOPED_RAND_RESET_I(8, __VscopeHash, 5834282488758667806ull);
    }
    for (int __Vi0 = 0; __Vi0 < 16384; ++__Vi0) {
        vlSelf->speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem[__Vi0] = VL_SCOPED_RAND_RESET_I(8, __VscopeHash, 7093423949353554219ull);
    }
    for (int __Vi0 = 0; __Vi0 < 16384; ++__Vi0) {
        vlSelf->speccy_tb_top__DOT__u_speccy__DOT__u_rom__DOT__mem[__Vi0] = VL_SCOPED_RAND_RESET_I(8, __VscopeHash, 16900592985652204735ull);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VstlTriggered[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->__VicoTriggered[__Vi0] = 0;
    }
    vlSelf->__Vtrigprevexpr___TOP__clk__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__rst__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cpu_a__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__cpu_do__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__mreq_n__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__iorq_n__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__rd_n__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__wr_n__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__m1_n__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__key_matrix__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__joy_state__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP__ear_in__0 = 0;
    vlSelf->__VicoDidInit = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VactTriggered[__Vi0] = 0;
    }
    vlSelf->__Vtrigprevexpr___TOP__clk__1 = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VnbaTriggered[__Vi0] = 0;
    }
}
