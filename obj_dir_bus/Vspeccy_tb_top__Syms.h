// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table internal header
//
// Internal details; most calling programs do not need this header,
// unless using verilator public meta comments.

#ifndef VERILATED_VSPECCY_TB_TOP__SYMS_H_
#define VERILATED_VSPECCY_TB_TOP__SYMS_H_  // guard

#include "verilated.h"

// INCLUDE MODEL CLASS

#include "Vspeccy_tb_top.h"

// INCLUDE MODULE CLASSES
#include "Vspeccy_tb_top___024root.h"

// SYMS CLASS (contains all model state)
class alignas(VL_CACHE_LINE_BYTES) Vspeccy_tb_top__Syms final : public VerilatedSyms {
  public:
    // INTERNAL STATE
    Vspeccy_tb_top* const __Vm_modelp;
    VlDeleter __Vm_deleter;
    bool __Vm_didInit = false;

    // MODULE INSTANCE STATE
    Vspeccy_tb_top___024root       TOP;

    // CONSTRUCTORS
    Vspeccy_tb_top__Syms(VerilatedContext* contextp, const char* namep, Vspeccy_tb_top* modelp);
    ~Vspeccy_tb_top__Syms();

    // METHODS
    const char* name() const { return TOP.vlNamep; }
};

#endif  // guard
