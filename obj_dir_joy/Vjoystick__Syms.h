// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table internal header
//
// Internal details; most calling programs do not need this header,
// unless using verilator public meta comments.

#ifndef VERILATED_VJOYSTICK__SYMS_H_
#define VERILATED_VJOYSTICK__SYMS_H_  // guard

#include "verilated.h"

// INCLUDE MODEL CLASS

#include "Vjoystick.h"

// INCLUDE MODULE CLASSES
#include "Vjoystick___024root.h"

// SYMS CLASS (contains all model state)
class alignas(VL_CACHE_LINE_BYTES) Vjoystick__Syms final : public VerilatedSyms {
  public:
    // INTERNAL STATE
    Vjoystick* const __Vm_modelp;
    VlDeleter __Vm_deleter;
    bool __Vm_didInit = false;

    // MODULE INSTANCE STATE
    Vjoystick___024root            TOP;

    // CONSTRUCTORS
    Vjoystick__Syms(VerilatedContext* contextp, const char* namep, Vjoystick* modelp);
    ~Vjoystick__Syms();

    // METHODS
    const char* name() const { return TOP.vlNamep; }
};

#endif  // guard
