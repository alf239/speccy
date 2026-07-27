// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vspeccy_tb_top.h for the primary calling header

#include "Vspeccy_tb_top__pch.h"

void Vspeccy_tb_top___024root___eval_triggers_vec__ico(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___eval_triggers_vec__ico\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__VicoTriggered[0U] = (QData)((IData)(
                                                    (((((((IData)(vlSelfRef.ear_in) 
                                                          != (IData)(vlSelfRef.__Vtrigprevexpr___TOP__ear_in__0)) 
                                                         << 3U) 
                                                        | (((IData)(vlSelfRef.joy_state) 
                                                            != (IData)(vlSelfRef.__Vtrigprevexpr___TOP__joy_state__0)) 
                                                           << 2U)) 
                                                       | (((vlSelfRef.key_matrix 
                                                            != vlSelfRef.__Vtrigprevexpr___TOP__key_matrix__0) 
                                                           << 1U) 
                                                          | ((IData)(vlSelfRef.m1_n) 
                                                             != (IData)(vlSelfRef.__Vtrigprevexpr___TOP__m1_n__0)))) 
                                                      << 8U) 
                                                     | (((((((IData)(vlSelfRef.wr_n) 
                                                             != (IData)(vlSelfRef.__Vtrigprevexpr___TOP__wr_n__0)) 
                                                            << 3U) 
                                                           | (((IData)(vlSelfRef.rd_n) 
                                                               != (IData)(vlSelfRef.__Vtrigprevexpr___TOP__rd_n__0)) 
                                                              << 2U)) 
                                                          | ((((IData)(vlSelfRef.iorq_n) 
                                                               != (IData)(vlSelfRef.__Vtrigprevexpr___TOP__iorq_n__0)) 
                                                              << 1U) 
                                                             | ((IData)(vlSelfRef.mreq_n) 
                                                                != (IData)(vlSelfRef.__Vtrigprevexpr___TOP__mreq_n__0)))) 
                                                         << 4U) 
                                                        | (((((IData)(vlSelfRef.cpu_do) 
                                                              != (IData)(vlSelfRef.__Vtrigprevexpr___TOP__cpu_do__0)) 
                                                             << 3U) 
                                                            | (((IData)(vlSelfRef.cpu_a) 
                                                                != (IData)(vlSelfRef.__Vtrigprevexpr___TOP__cpu_a__0)) 
                                                               << 2U)) 
                                                           | ((((IData)(vlSelfRef.rst) 
                                                                != (IData)(vlSelfRef.__Vtrigprevexpr___TOP__rst__0)) 
                                                               << 1U) 
                                                              | ((IData)(vlSelfRef.clk) 
                                                                 != (IData)(vlSelfRef.__Vtrigprevexpr___TOP__clk__0))))))));
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
    if (VL_UNLIKELY(((1U & (~ (IData)(vlSelfRef.__VicoDidInit)))))) {
        vlSelfRef.__VicoDidInit = 1U;
        vlSelfRef.__VicoTriggered[0U] = (1ULL | vlSelfRef.__VicoTriggered[0U]);
        vlSelfRef.__VicoTriggered[0U] = (2ULL | vlSelfRef.__VicoTriggered[0U]);
        vlSelfRef.__VicoTriggered[0U] = (4ULL | vlSelfRef.__VicoTriggered[0U]);
        vlSelfRef.__VicoTriggered[0U] = (8ULL | vlSelfRef.__VicoTriggered[0U]);
        vlSelfRef.__VicoTriggered[0U] = (0x0000000000000010ULL 
                                         | vlSelfRef.__VicoTriggered[0U]);
        vlSelfRef.__VicoTriggered[0U] = (0x0000000000000020ULL 
                                         | vlSelfRef.__VicoTriggered[0U]);
        vlSelfRef.__VicoTriggered[0U] = (0x0000000000000040ULL 
                                         | vlSelfRef.__VicoTriggered[0U]);
        vlSelfRef.__VicoTriggered[0U] = (0x0000000000000080ULL 
                                         | vlSelfRef.__VicoTriggered[0U]);
        vlSelfRef.__VicoTriggered[0U] = (0x0000000000000100ULL 
                                         | vlSelfRef.__VicoTriggered[0U]);
        vlSelfRef.__VicoTriggered[0U] = (0x0000000000000200ULL 
                                         | vlSelfRef.__VicoTriggered[0U]);
        vlSelfRef.__VicoTriggered[0U] = (0x0000000000000400ULL 
                                         | vlSelfRef.__VicoTriggered[0U]);
        vlSelfRef.__VicoTriggered[0U] = (0x0000000000000800ULL 
                                         | vlSelfRef.__VicoTriggered[0U]);
    }
}

bool Vspeccy_tb_top___024root___trigger_anySet__ico(const VlUnpacked<QData/*63:0*/, 2> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___trigger_anySet__ico\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        if (in[n]) {
            return (1U);
        }
        n = ((IData)(1U) + n);
    } while ((2U > n));
    return (0U);
}

extern const VlUnpacked<CData/*7:0*/, 32> Vspeccy_tb_top__ConstPool__TABLE_hc7e26bc0_0;

void Vspeccy_tb_top___024root___eval_ico(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___eval_ico\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if ((0x0000000000000090ULL & vlSelfRef.__VicoTriggered[0U])) {
        {
            // Inlined CFunc: _ico_comb__TOP__0
            vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__mem_wr 
                = (1U & ((~ (IData)(vlSelfRef.mreq_n)) 
                         & (~ (IData)(vlSelfRef.wr_n))));
        }
    }
    if ((0x00000000000001a0ULL & vlSelfRef.__VicoTriggered[0U])) {
        {
            // Inlined CFunc: _ico_comb__TOP__1
            vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__io_wr 
                = ((~ (IData)(vlSelfRef.iorq_n)) & 
                   ((~ (IData)(vlSelfRef.wr_n)) & (IData)(vlSelfRef.m1_n)));
        }
    }
    if ((0x0000000000000204ULL & vlSelfRef.__VicoTriggered[0U])) {
        {
            // Inlined CFunc: _ico_comb__TOP__2
            vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed = 0U;
            if ((1U & (~ ((IData)(vlSelfRef.cpu_a) 
                          >> 8U)))) {
                vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
                    = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                                      | (IData)(vlSelfRef.key_matrix)));
            }
            if ((1U & (~ ((IData)(vlSelfRef.cpu_a) 
                          >> 9U)))) {
                vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
                    = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                                      | (IData)((vlSelfRef.key_matrix 
                                                 >> 5U))));
            }
            if ((1U & (~ ((IData)(vlSelfRef.cpu_a) 
                          >> 0x0000000aU)))) {
                vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
                    = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                                      | (IData)((vlSelfRef.key_matrix 
                                                 >> 0x0aU))));
            }
            if ((1U & (~ ((IData)(vlSelfRef.cpu_a) 
                          >> 0x0000000bU)))) {
                vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
                    = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                                      | (IData)((vlSelfRef.key_matrix 
                                                 >> 0x0fU))));
            }
            if ((1U & (~ ((IData)(vlSelfRef.cpu_a) 
                          >> 0x0000000cU)))) {
                vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
                    = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                                      | (IData)((vlSelfRef.key_matrix 
                                                 >> 0x14U))));
            }
            if ((1U & (~ ((IData)(vlSelfRef.cpu_a) 
                          >> 0x0000000dU)))) {
                vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
                    = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                                      | (IData)((vlSelfRef.key_matrix 
                                                 >> 0x19U))));
            }
            if ((1U & (~ ((IData)(vlSelfRef.cpu_a) 
                          >> 0x0000000eU)))) {
                vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
                    = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                                      | (IData)((vlSelfRef.key_matrix 
                                                 >> 0x1eU))));
            }
            if ((1U & (~ ((IData)(vlSelfRef.cpu_a) 
                          >> 0x0000000fU)))) {
                vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed 
                    = (0x0000001fU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed) 
                                      | (IData)((vlSelfRef.key_matrix 
                                                 >> 0x23U))));
            }
        }
    }
    if ((0x0000000000000f74ULL & vlSelfRef.__VicoTriggered[0U])) {
        {
            // Inlined CFunc: _ico_comb__TOP__3
            vlSelfRef.cpu_di = (0x000000ffU & ((1U 
                                                & ((~ (IData)(vlSelfRef.mreq_n)) 
                                                   & (~ (IData)(vlSelfRef.rd_n))))
                                                ? (
                                                   (0U 
                                                    == (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__rd_bank))
                                                    ? (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__rom_q)
                                                    : 
                                                   ((1U 
                                                     == (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__rd_bank))
                                                     ? (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__vram_q)
                                                     : (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__ramhi_q)))
                                                : (
                                                   (- (IData)(
                                                              (1U 
                                                               & (~ 
                                                                  ((~ (IData)(vlSelfRef.iorq_n)) 
                                                                   & ((~ (IData)(vlSelfRef.rd_n)) 
                                                                      & (IData)(vlSelfRef.m1_n))))))) 
                                                   | ((1U 
                                                       & (IData)(vlSelfRef.cpu_a))
                                                       ? 
                                                      (Vspeccy_tb_top__ConstPool__TABLE_hc7e26bc0_0
                                                       [vlSelfRef.joy_state] 
                                                       | (- (IData)(
                                                                    (1U 
                                                                     & ((IData)(vlSelfRef.cpu_a) 
                                                                        >> 5U)))))
                                                       : 
                                                      (0x000000a0U 
                                                       | (((IData)(vlSelfRef.ear_in) 
                                                           << 6U) 
                                                          | (0x0000001fU 
                                                             & (~ (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed)))))))));
        }
    }
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vspeccy_tb_top___024root___dump_triggers__ico(const VlUnpacked<QData/*63:0*/, 2> &triggers, const std::string &tag);
#endif  // VL_DEBUG

bool Vspeccy_tb_top___024root___eval_phase__ico(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___eval_phase__ico\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VicoExecute;
    // Body
    Vspeccy_tb_top___024root___eval_triggers_vec__ico(vlSelf);
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vspeccy_tb_top___024root___dump_triggers__ico(vlSelfRef.__VicoTriggered, "ico"s);
    }
#endif
    __VicoExecute = Vspeccy_tb_top___024root___trigger_anySet__ico(vlSelfRef.__VicoTriggered);
    if (__VicoExecute) {
        Vspeccy_tb_top___024root___eval_ico(vlSelf);
    }
    return (__VicoExecute);
}

bool Vspeccy_tb_top___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___trigger_anySet__act\n"); );
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

void Vspeccy_tb_top___024root___nba_sequent__TOP__0(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___nba_sequent__TOP__0\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    SData/*8:0*/ speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__fh;
    speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__fh = 0;
    SData/*8:0*/ speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__fv;
    speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__fv = 0;
    CData/*7:0*/ __Vdly__speccy_tb_top__DOT__u_speccy__DOT__int_ctr;
    __Vdly__speccy_tb_top__DOT__u_speccy__DOT__int_ctr = 0;
    SData/*8:0*/ __Vdly__speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc;
    __Vdly__speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc = 0;
    SData/*8:0*/ __Vdly__speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc;
    __Vdly__speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc = 0;
    CData/*7:0*/ __VdlyVal__speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem__v0;
    __VdlyVal__speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem__v0 = 0;
    SData/*14:0*/ __VdlyDim0__speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem__v0;
    __VdlyDim0__speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem__v0 = 0;
    CData/*0:0*/ __VdlySet__speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem__v0;
    __VdlySet__speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem__v0 = 0;
    CData/*7:0*/ __VdlyVal__speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem__v0;
    __VdlyVal__speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem__v0 = 0;
    SData/*13:0*/ __VdlyDim0__speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem__v0;
    __VdlyDim0__speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem__v0 = 0;
    CData/*0:0*/ __VdlySet__speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem__v0;
    __VdlySet__speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem__v0 = 0;
    // Body
    __VdlySet__speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem__v0 = 0U;
    __VdlySet__speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem__v0 = 0U;
    __Vdly__speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc 
        = vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc;
    __Vdly__speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc 
        = vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc;
    __Vdly__speccy_tb_top__DOT__u_speccy__DOT__int_ctr 
        = vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__int_ctr;
    if (vlSelfRef.rst) {
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__phase = 0U;
        __Vdly__speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc = 0U;
        __Vdly__speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc = 0U;
        __Vdly__speccy_tb_top__DOT__u_speccy__DOT__int_ctr = 0U;
        vlSelfRef.speaker = 0U;
        vlSelfRef.mic = 0U;
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__border_r = 0U;
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__px_v = 0U;
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__px_h = 0U;
    } else {
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__phase 
            = (3U & ((IData)(1U) + (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__phase)));
        if (((IData)(vlSelfRef.ce_pix) & ((0x00f8U 
                                           == (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__px_v)) 
                                          & (0U == (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__px_h))))) {
            __Vdly__speccy_tb_top__DOT__u_speccy__DOT__int_ctr = 0x20U;
        } else if (((IData)(vlSelfRef.ce_cpu) & (0U 
                                                 != (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__int_ctr)))) {
            __Vdly__speccy_tb_top__DOT__u_speccy__DOT__int_ctr 
                = (0x000000ffU & ((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__int_ctr) 
                                  - (IData)(1U)));
        }
        if (vlSelfRef.ce_pix) {
            if ((0x01bfU == (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc))) {
                __Vdly__speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc 
                    = ((0x0137U == (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc))
                        ? 0U : (0x000001ffU & ((IData)(1U) 
                                               + (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc))));
                __Vdly__speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc = 0U;
            } else {
                __Vdly__speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc 
                    = (0x000001ffU & ((IData)(1U) + (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc)));
            }
            vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__px_v 
                = vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc;
            vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__px_h 
                = vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc;
        }
        if (((IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__io_wr) 
             & (~ (IData)(vlSelfRef.cpu_a)))) {
            vlSelfRef.speaker = (1U & ((IData)(vlSelfRef.cpu_do) 
                                       >> 4U));
            vlSelfRef.mic = (1U & ((IData)(vlSelfRef.cpu_do) 
                                   >> 3U));
            vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__border_r 
                = (7U & (IData)(vlSelfRef.cpu_do));
        }
    }
    if ((((IData)(vlSelfRef.cpu_a) >> 0x0000000fU) 
         & (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__mem_wr))) {
        __VdlyVal__speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem__v0 
            = vlSelfRef.cpu_do;
        __VdlyDim0__speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem__v0 
            = (0x00007fffU & (IData)(vlSelfRef.cpu_a));
        __VdlySet__speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem__v0 = 1U;
    }
    if ((IData)(((0x4000U == (0xc000U & (IData)(vlSelfRef.cpu_a))) 
                 & (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__mem_wr)))) {
        __VdlyVal__speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem__v0 
            = vlSelfRef.cpu_do;
        __VdlyDim0__speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem__v0 
            = (0x00003fffU & (IData)(vlSelfRef.cpu_a));
        __VdlySet__speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem__v0 = 1U;
    }
    vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__rd_bank 
        = (3U & ((IData)(vlSelfRef.cpu_a) >> 0x0eU));
    vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__rom_q 
        = vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_rom__DOT__mem
        [(0x00003fffU & (IData)(vlSelfRef.cpu_a))];
    vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__ramhi_q 
        = vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem
        [(0x00007fffU & (IData)(vlSelfRef.cpu_a))];
    vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__vram_q 
        = vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem
        [(0x00003fffU & (IData)(vlSelfRef.cpu_a))];
    vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__int_ctr 
        = __Vdly__speccy_tb_top__DOT__u_speccy__DOT__int_ctr;
    if (__VdlySet__speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem__v0) {
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem[__VdlyDim0__speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem__v0] 
            = __VdlyVal__speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem__v0;
    }
    if (__VdlySet__speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem__v0) {
        vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem[__VdlyDim0__speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem__v0] 
            = __VdlyVal__speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem__v0;
    }
    vlSelfRef.ce_cpu = (3U == (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__phase));
    vlSelfRef.int_n = (0U == (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__int_ctr));
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
    vlSelfRef.border = vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__border_r;
    vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc 
        = __Vdly__speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc;
    vlSelfRef.ce_pix = (1U & (IData)(vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__phase));
    vlSelfRef.speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc 
        = __Vdly__speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc;
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
}

void Vspeccy_tb_top___024root___trigger_orInto__act_vec_vec(VlUnpacked<QData/*63:0*/, 1> &out, const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___trigger_orInto__act_vec_vec\n"); );
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
VL_ATTR_COLD void Vspeccy_tb_top___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG

bool Vspeccy_tb_top___024root___eval_phase__act(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___eval_phase__act\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    {
        // Inlined CFunc: _eval_triggers_vec__act
        vlSelfRef.__VactTriggered[0U] = (QData)((IData)(
                                                        ((IData)(vlSelfRef.clk) 
                                                         & (~ (IData)(vlSelfRef.__Vtrigprevexpr___TOP__clk__1)))));
        vlSelfRef.__Vtrigprevexpr___TOP__clk__1 = vlSelfRef.clk;
    }
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vspeccy_tb_top___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
    }
#endif
    Vspeccy_tb_top___024root___trigger_orInto__act_vec_vec(vlSelfRef.__VnbaTriggered, vlSelfRef.__VactTriggered);
    return (0U);
}

void Vspeccy_tb_top___024root___trigger_clear__act(VlUnpacked<QData/*63:0*/, 1> &out) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___trigger_clear__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        out[n] = 0ULL;
        n = ((IData)(1U) + n);
    } while ((1U > n));
}

bool Vspeccy_tb_top___024root___eval_phase__nba(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___eval_phase__nba\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VnbaExecute;
    // Body
    __VnbaExecute = Vspeccy_tb_top___024root___trigger_anySet__act(vlSelfRef.__VnbaTriggered);
    if (__VnbaExecute) {
        {
            // Inlined CFunc: _eval_nba
            if ((1ULL & vlSelfRef.__VnbaTriggered[0U])) {
                Vspeccy_tb_top___024root___nba_sequent__TOP__0(vlSelf);
            }
        }
        Vspeccy_tb_top___024root___trigger_clear__act(vlSelfRef.__VnbaTriggered);
    }
    return (__VnbaExecute);
}

void Vspeccy_tb_top___024root___eval(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___eval\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VicoIterCount;
    IData/*31:0*/ __VnbaIterCount;
    // Body
    __VicoIterCount = 0U;
    do {
        if (VL_UNLIKELY(((0x00002710U < __VicoIterCount)))) {
#ifdef VL_DEBUG
            Vspeccy_tb_top___024root___dump_triggers__ico(vlSelfRef.__VicoTriggered, "ico"s);
#endif
            VL_FATAL_MT("sim/speccy_tb_top.v", 6, "", "DIDNOTCONVERGE: Input combinational region did not converge after '--converge-limit' of 10000 tries");
        }
        __VicoIterCount = ((IData)(1U) + __VicoIterCount);
        vlSelfRef.__VicoPhaseResult = Vspeccy_tb_top___024root___eval_phase__ico(vlSelf);
    } while (vlSelfRef.__VicoPhaseResult);
    __VnbaIterCount = 0U;
    do {
        if (VL_UNLIKELY(((0x00002710U < __VnbaIterCount)))) {
#ifdef VL_DEBUG
            Vspeccy_tb_top___024root___dump_triggers__act(vlSelfRef.__VnbaTriggered, "nba"s);
#endif
            VL_FATAL_MT("sim/speccy_tb_top.v", 6, "", "DIDNOTCONVERGE: NBA region did not converge after '--converge-limit' of 10000 tries");
        }
        __VnbaIterCount = ((IData)(1U) + __VnbaIterCount);
        vlSelfRef.__VactIterCount = 0U;
        do {
            if (VL_UNLIKELY(((0x00002710U < vlSelfRef.__VactIterCount)))) {
#ifdef VL_DEBUG
                Vspeccy_tb_top___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
#endif
                VL_FATAL_MT("sim/speccy_tb_top.v", 6, "", "DIDNOTCONVERGE: Active region did not converge after '--converge-limit' of 10000 tries");
            }
            vlSelfRef.__VactIterCount = ((IData)(1U) 
                                         + vlSelfRef.__VactIterCount);
            vlSelfRef.__VactPhaseResult = Vspeccy_tb_top___024root___eval_phase__act(vlSelf);
        } while (vlSelfRef.__VactPhaseResult);
        vlSelfRef.__VnbaPhaseResult = Vspeccy_tb_top___024root___eval_phase__nba(vlSelf);
    } while (vlSelfRef.__VnbaPhaseResult);
}

#ifdef VL_DEBUG
void Vspeccy_tb_top___024root___eval_debug_assertions(Vspeccy_tb_top___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vspeccy_tb_top___024root___eval_debug_assertions\n"); );
    Vspeccy_tb_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if (VL_UNLIKELY(((vlSelfRef.clk & 0xfeU)))) {
        Verilated::overWidthError("clk");
    }
    if (VL_UNLIKELY(((vlSelfRef.rst & 0xfeU)))) {
        Verilated::overWidthError("rst");
    }
    if (VL_UNLIKELY(((vlSelfRef.mreq_n & 0xfeU)))) {
        Verilated::overWidthError("mreq_n");
    }
    if (VL_UNLIKELY(((vlSelfRef.iorq_n & 0xfeU)))) {
        Verilated::overWidthError("iorq_n");
    }
    if (VL_UNLIKELY(((vlSelfRef.rd_n & 0xfeU)))) {
        Verilated::overWidthError("rd_n");
    }
    if (VL_UNLIKELY(((vlSelfRef.wr_n & 0xfeU)))) {
        Verilated::overWidthError("wr_n");
    }
    if (VL_UNLIKELY(((vlSelfRef.m1_n & 0xfeU)))) {
        Verilated::overWidthError("m1_n");
    }
    if (VL_UNLIKELY(((vlSelfRef.key_matrix & 0ULL)))) {
        Verilated::overWidthError("key_matrix");
    }
    if (VL_UNLIKELY(((vlSelfRef.joy_state & 0xe0U)))) {
        Verilated::overWidthError("joy_state");
    }
    if (VL_UNLIKELY(((vlSelfRef.ear_in & 0xfeU)))) {
        Verilated::overWidthError("ear_in");
    }
}
#endif  // VL_DEBUG
