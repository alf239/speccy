// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Model implementation (design independent parts)

#include "Vspeccy_tb_top__pch.h"

//============================================================
// Constructors

Vspeccy_tb_top::Vspeccy_tb_top(VerilatedContext* _vcontextp__, const char* _vcname__)
    : VerilatedModel{*_vcontextp__}
    , vlSymsp{new Vspeccy_tb_top__Syms(contextp(), _vcname__, this)}
    , clk{vlSymsp->TOP.clk}
    , rst{vlSymsp->TOP.rst}
    , ce_cpu{vlSymsp->TOP.ce_cpu}
    , ce_pix{vlSymsp->TOP.ce_pix}
    , cpu_do{vlSymsp->TOP.cpu_do}
    , cpu_di{vlSymsp->TOP.cpu_di}
    , mreq_n{vlSymsp->TOP.mreq_n}
    , iorq_n{vlSymsp->TOP.iorq_n}
    , rd_n{vlSymsp->TOP.rd_n}
    , wr_n{vlSymsp->TOP.wr_n}
    , m1_n{vlSymsp->TOP.m1_n}
    , int_n{vlSymsp->TOP.int_n}
    , joy_state{vlSymsp->TOP.joy_state}
    , ear_in{vlSymsp->TOP.ear_in}
    , speaker{vlSymsp->TOP.speaker}
    , mic{vlSymsp->TOP.mic}
    , border{vlSymsp->TOP.border}
    , cpu_a{vlSymsp->TOP.cpu_a}
    , key_matrix{vlSymsp->TOP.key_matrix}
    , rootp{&(vlSymsp->TOP)}
{
    // Register model with the context
    contextp()->addModel(this);
}

Vspeccy_tb_top::Vspeccy_tb_top(const char* _vcname__)
    : Vspeccy_tb_top(Verilated::threadContextp(), _vcname__)
{
}

//============================================================
// Destructor

Vspeccy_tb_top::~Vspeccy_tb_top() {
    delete vlSymsp;
}

//============================================================
// Evaluation function

#ifdef VL_DEBUG
void Vspeccy_tb_top___024root___eval_debug_assertions(Vspeccy_tb_top___024root* vlSelf);
#endif  // VL_DEBUG
void Vspeccy_tb_top___024root___eval_static(Vspeccy_tb_top___024root* vlSelf);
void Vspeccy_tb_top___024root___eval_initial(Vspeccy_tb_top___024root* vlSelf);
void Vspeccy_tb_top___024root___eval_settle(Vspeccy_tb_top___024root* vlSelf);
void Vspeccy_tb_top___024root___eval(Vspeccy_tb_top___024root* vlSelf);

void Vspeccy_tb_top::eval_step() {
    VL_DEBUG_IF(VL_DBG_MSGF("+++++TOP Evaluate Vspeccy_tb_top::eval_step\n"); );
#ifdef VL_DEBUG
    // Debug assertions
    Vspeccy_tb_top___024root___eval_debug_assertions(&(vlSymsp->TOP));
#endif  // VL_DEBUG
    vlSymsp->__Vm_deleter.deleteAll();
    if (VL_UNLIKELY(!vlSymsp->__Vm_didInit)) {
        VL_DEBUG_IF(VL_DBG_MSGF("+ Initial\n"););
        Vspeccy_tb_top___024root___eval_static(&(vlSymsp->TOP));
        Vspeccy_tb_top___024root___eval_initial(&(vlSymsp->TOP));
        Vspeccy_tb_top___024root___eval_settle(&(vlSymsp->TOP));
        vlSymsp->__Vm_didInit = true;
    }
    VL_DEBUG_IF(VL_DBG_MSGF("+ Eval\n"););
    Vspeccy_tb_top___024root___eval(&(vlSymsp->TOP));
    // Evaluate cleanup
    Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);
}

//============================================================
// Events and timing
bool Vspeccy_tb_top::eventsPending() { return false; }

uint64_t Vspeccy_tb_top::nextTimeSlot() {
    VL_FATAL_MT(__FILE__, __LINE__, "", "No delays in the design");
    return 0;
}

//============================================================
// Utilities

const char* Vspeccy_tb_top::name() const {
    return vlSymsp->name();
}

//============================================================
// Invoke final blocks

void Vspeccy_tb_top___024root___eval_final(Vspeccy_tb_top___024root* vlSelf);

VL_ATTR_COLD void Vspeccy_tb_top::final() {
    contextp()->executingFinal(true);
    Vspeccy_tb_top___024root___eval_final(&(vlSymsp->TOP));
    contextp()->executingFinal(false);
}

//============================================================
// Implementations of abstract methods from VerilatedModel

const char* Vspeccy_tb_top::hierName() const { return vlSymsp->name(); }
const char* Vspeccy_tb_top::modelName() const { return "Vspeccy_tb_top"; }
unsigned Vspeccy_tb_top::threads() const { return 1; }
void Vspeccy_tb_top::prepareClone() const { contextp()->prepareClone(); }
void Vspeccy_tb_top::atClone() const {
    contextp()->threadPoolpOnClone();
}
