// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vspeccy_tb_top.h for the primary calling header

#ifndef VERILATED_VSPECCY_TB_TOP___024ROOT_H_
#define VERILATED_VSPECCY_TB_TOP___024ROOT_H_  // guard

#include "verilated.h"


class Vspeccy_tb_top__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vspeccy_tb_top___024root final {
  public:

    // DESIGN SPECIFIC STATE
    VL_IN8(clk,0,0);
    VL_IN8(rst,0,0);
    VL_OUT8(ce_cpu,0,0);
    VL_OUT8(ce_pix,0,0);
    VL_IN8(cpu_do,7,0);
    VL_OUT8(cpu_di,7,0);
    VL_IN8(mreq_n,0,0);
    VL_IN8(iorq_n,0,0);
    VL_IN8(rd_n,0,0);
    VL_IN8(wr_n,0,0);
    VL_IN8(m1_n,0,0);
    VL_OUT8(int_n,0,0);
    VL_IN8(joy_state,4,0);
    VL_IN8(ear_in,0,0);
    VL_OUT8(speaker,0,0);
    VL_OUT8(mic,0,0);
    VL_OUT8(border,2,0);
    CData/*1:0*/ speccy_tb_top__DOT__u_speccy__DOT__phase;
    CData/*0:0*/ speccy_tb_top__DOT__u_speccy__DOT__mem_wr;
    CData/*7:0*/ speccy_tb_top__DOT__u_speccy__DOT__rom_q;
    CData/*7:0*/ speccy_tb_top__DOT__u_speccy__DOT__vram_q;
    CData/*7:0*/ speccy_tb_top__DOT__u_speccy__DOT__ramhi_q;
    CData/*1:0*/ speccy_tb_top__DOT__u_speccy__DOT__rd_bank;
    CData/*0:0*/ speccy_tb_top__DOT__u_speccy__DOT__io_wr;
    CData/*2:0*/ speccy_tb_top__DOT__u_speccy__DOT__border_r;
    CData/*7:0*/ speccy_tb_top__DOT__u_speccy__DOT__int_ctr;
    CData/*4:0*/ speccy_tb_top__DOT__u_speccy__DOT__u_keyboard__DOT__pressed;
    CData/*0:0*/ __VstlFirstIteration;
    CData/*0:0*/ __VstlPhaseResult;
    CData/*0:0*/ __Vtrigprevexpr___TOP__clk__0;
    CData/*0:0*/ __Vtrigprevexpr___TOP__rst__0;
    CData/*7:0*/ __Vtrigprevexpr___TOP__cpu_do__0;
    CData/*0:0*/ __Vtrigprevexpr___TOP__mreq_n__0;
    CData/*0:0*/ __Vtrigprevexpr___TOP__iorq_n__0;
    CData/*0:0*/ __Vtrigprevexpr___TOP__rd_n__0;
    CData/*0:0*/ __Vtrigprevexpr___TOP__wr_n__0;
    CData/*0:0*/ __Vtrigprevexpr___TOP__m1_n__0;
    CData/*4:0*/ __Vtrigprevexpr___TOP__joy_state__0;
    CData/*0:0*/ __Vtrigprevexpr___TOP__ear_in__0;
    CData/*0:0*/ __VicoDidInit;
    CData/*0:0*/ __VicoPhaseResult;
    CData/*0:0*/ __Vtrigprevexpr___TOP__clk__1;
    CData/*0:0*/ __VactPhaseResult;
    CData/*0:0*/ __VnbaPhaseResult;
    VL_IN16(cpu_a,15,0);
    SData/*8:0*/ speccy_tb_top__DOT__u_speccy__DOT__px_h;
    SData/*8:0*/ speccy_tb_top__DOT__u_speccy__DOT__px_v;
    SData/*8:0*/ speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__hc;
    SData/*8:0*/ speccy_tb_top__DOT__u_speccy__DOT__u_video__DOT__vc;
    SData/*15:0*/ __Vtrigprevexpr___TOP__cpu_a__0;
    IData/*31:0*/ __VactIterCount;
    VL_IN64(key_matrix,39,0);
    QData/*39:0*/ __Vtrigprevexpr___TOP__key_matrix__0;
    VlUnpacked<CData/*7:0*/, 32768> speccy_tb_top__DOT__u_speccy__DOT__u_ramhi__DOT__mem;
    VlUnpacked<CData/*7:0*/, 16384> speccy_tb_top__DOT__u_speccy__DOT__u_vram__DOT__mem;
    VlUnpacked<CData/*7:0*/, 16384> speccy_tb_top__DOT__u_speccy__DOT__u_rom__DOT__mem;
    VlUnpacked<QData/*63:0*/, 1> __VstlTriggered;
    VlUnpacked<QData/*63:0*/, 2> __VicoTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VactTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VnbaTriggered;

    // INTERNAL VARIABLES
    Vspeccy_tb_top__Syms* vlSymsp;
    const char* vlNamep;

    // CONSTRUCTORS
    Vspeccy_tb_top___024root(Vspeccy_tb_top__Syms* symsp, const char* namep);
    ~Vspeccy_tb_top___024root();
    VL_UNCOPYABLE(Vspeccy_tb_top___024root);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
