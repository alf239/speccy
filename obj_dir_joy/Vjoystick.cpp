// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Model implementation (design independent parts)

#include "Vjoystick__pch.h"

//============================================================
// Constructors

Vjoystick::Vjoystick(VerilatedContext* _vcontextp__, const char* _vcname__)
    : VerilatedModel{*_vcontextp__}
    , vlSymsp{new Vjoystick__Syms(contextp(), _vcname__, this)}
    , clk{vlSymsp->TOP.clk}
    , rst{vlSymsp->TOP.rst}
    , pin_n{vlSymsp->TOP.pin_n}
    , state{vlSymsp->TOP.state}
    , kempston{vlSymsp->TOP.kempston}
    , rootp{&(vlSymsp->TOP)}
{
    // Register model with the context
    contextp()->addModel(this);
}

Vjoystick::Vjoystick(const char* _vcname__)
    : Vjoystick(Verilated::threadContextp(), _vcname__)
{
}

//============================================================
// Destructor

Vjoystick::~Vjoystick() {
    delete vlSymsp;
}

//============================================================
// Evaluation function

#ifdef VL_DEBUG
void Vjoystick___024root___eval_debug_assertions(Vjoystick___024root* vlSelf);
#endif  // VL_DEBUG
void Vjoystick___024root___eval_static(Vjoystick___024root* vlSelf);
void Vjoystick___024root___eval_initial(Vjoystick___024root* vlSelf);
void Vjoystick___024root___eval_settle(Vjoystick___024root* vlSelf);
void Vjoystick___024root___eval(Vjoystick___024root* vlSelf);

void Vjoystick::eval_step() {
    VL_DEBUG_IF(VL_DBG_MSGF("+++++TOP Evaluate Vjoystick::eval_step\n"); );
#ifdef VL_DEBUG
    // Debug assertions
    Vjoystick___024root___eval_debug_assertions(&(vlSymsp->TOP));
#endif  // VL_DEBUG
    vlSymsp->__Vm_deleter.deleteAll();
    if (VL_UNLIKELY(!vlSymsp->__Vm_didInit)) {
        VL_DEBUG_IF(VL_DBG_MSGF("+ Initial\n"););
        Vjoystick___024root___eval_static(&(vlSymsp->TOP));
        Vjoystick___024root___eval_initial(&(vlSymsp->TOP));
        Vjoystick___024root___eval_settle(&(vlSymsp->TOP));
        vlSymsp->__Vm_didInit = true;
    }
    VL_DEBUG_IF(VL_DBG_MSGF("+ Eval\n"););
    Vjoystick___024root___eval(&(vlSymsp->TOP));
    // Evaluate cleanup
    Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);
}

//============================================================
// Events and timing
bool Vjoystick::eventsPending() { return false; }

uint64_t Vjoystick::nextTimeSlot() {
    VL_FATAL_MT(__FILE__, __LINE__, "", "No delays in the design");
    return 0;
}

//============================================================
// Utilities

const char* Vjoystick::name() const {
    return vlSymsp->name();
}

//============================================================
// Invoke final blocks

void Vjoystick___024root___eval_final(Vjoystick___024root* vlSelf);

VL_ATTR_COLD void Vjoystick::final() {
    contextp()->executingFinal(true);
    Vjoystick___024root___eval_final(&(vlSymsp->TOP));
    contextp()->executingFinal(false);
}

//============================================================
// Implementations of abstract methods from VerilatedModel

const char* Vjoystick::hierName() const { return vlSymsp->name(); }
const char* Vjoystick::modelName() const { return "Vjoystick"; }
unsigned Vjoystick::threads() const { return 1; }
void Vjoystick::prepareClone() const { contextp()->prepareClone(); }
void Vjoystick::atClone() const {
    contextp()->threadPoolpOnClone();
}
