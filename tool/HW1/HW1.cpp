#include "pin.H"
#include <iostream>
#include <fstream>
#include <stdlib.h>

using std::cerr;
using std::endl;
using std::string;

// 1. Knobs
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "hw1.out", "specify output file name");
KNOB<UINT64> KnobFastForward(KNOB_MODE_WRITEONCE, "pintool", "f", "0", "number of billions of instructions to fast-forward");

std::ofstream OutFile;

// 2. Global Execution Timeline Variables
UINT64 icount = 0;
UINT64 ff_target = 0;
UINT64 end_target = 0;

// 3. Part A Counters (17 Categories)
UINT64 count_loads = 0;
UINT64 count_stores = 0;
UINT64 count_nops = 0;
UINT64 count_direct_calls = 0;
UINT64 count_indirect_calls = 0;
UINT64 count_returns = 0;
UINT64 count_uncond_br = 0;
UINT64 count_cond_br = 0;
UINT64 count_logical = 0;
UINT64 count_rotate_shift = 0;
UINT64 count_flag = 0;
UINT64 count_vector = 0;
UINT64 count_cmov = 0;
UINT64 count_mmx_sse = 0;
UINT64 count_syscall = 0;
UINT64 count_fp = 0;
UINT64 count_others = 0;

// 4. Analysis Routines (Execution Time - MUST BE LIGHTWEIGHT)
VOID InsCount() { 
    icount++; 
}

ADDRINT FastForward() {
    return (icount >= ff_target && icount < end_target);
}

ADDRINT TerminateCheck() {
    return (icount >= end_target);
}

// This gets called only if FastForward() returns true
VOID RecordInstruction(UINT64* counterToIncrement, UINT32 amount) {
    *counterToIncrement += amount;
}

VOID PrintStatsAndExit() {
    UINT64 total_type_A = count_nops + count_direct_calls + count_indirect_calls + count_returns + 
                          count_uncond_br + count_cond_br + count_logical + count_rotate_shift + 
                          count_flag + count_vector + count_cmov + count_mmx_sse + count_syscall + 
                          count_fp + count_others;
    
    UINT64 total_instructions = total_type_A + count_loads + count_stores;

    // Part B: CPI Calculation
    // Loads & Stores = 70 cycles, everything else = 1 cycle
    UINT64 total_cycles = ((count_loads + count_stores) * 70) + (total_type_A * 1);
    double cpi = (total_instructions > 0) ? (double)total_cycles / total_instructions : 0.0;

    OutFile << "===============================================" << endl;
    OutFile << "Total Instructions Executed (Timeline): " << icount << endl;
    OutFile << "Total Profiled Instructions (Denominator): " << total_instructions << endl;
    OutFile << "Calculated CPI: " << cpi << endl;
    OutFile << "===============================================" << endl;
    OutFile.close();
    exit(0);
}

// 5. Instrumentation Routine (JIT Compile Time - Heavy lifting goes here)
VOID Instruction(INS ins, VOID *v) {
    // 5a. Global Execution Timeline
    INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)TerminateCheck, IARG_END);
    INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)PrintStatsAndExit, IARG_END);
    
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)InsCount, IARG_END);

    // 5b. Memory Operands (Loads/Stores) Breakdown
    // The prof specified that every 32 bits (4 bytes) counts as 1 micro-op.
    UINT32 num_loads = 0;
    UINT32 num_stores = 0;
    
    if (INS_IsMemoryRead(ins) || INS_IsMemoryWrite(ins)) {
        UINT32 memOperands = INS_MemoryOperandCount(ins);
        for (UINT32 i = 0; i < memOperands; i++) {
            UINT32 size = INS_MemoryOperandSize(ins, i);
            UINT32 chunks = (size + 3) / 4; // Integer math trick for ceil(size / 4.0)

            if (INS_MemoryOperandIsRead(ins, i)) {
                num_loads += chunks;
            }
            if (INS_MemoryOperandIsWritten(ins, i)) {
                num_stores += chunks;
            }
        }
    }

    // Insert Predicated Calls to record Loads/Stores if they exist
    if (num_loads > 0) {
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordInstruction, IARG_PTR, &count_loads, IARG_UINT32, num_loads, IARG_END);
    }
    if (num_stores > 0) {
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordInstruction, IARG_PTR, &count_stores, IARG_UINT32, num_stores, IARG_END);
    }

    // 5c. Type A Instruction Categorization
    UINT64* target_counter = &count_others; // Default fallback
    
    INT32 category = INS_Category(ins);
    
    if (category == XED_CATEGORY_NOP) {
        target_counter = &count_nops;
    } else if (category == XED_CATEGORY_CALL) {
        target_counter = INS_IsDirectCall(ins) ? &count_direct_calls : &count_indirect_calls;
    } else if (category == XED_CATEGORY_RET) {
        target_counter = &count_returns;
    } else if (category == XED_CATEGORY_UNCOND_BR) {
        target_counter = &count_uncond_br;
    } else if (category == XED_CATEGORY_COND_BR) {
        target_counter = &count_cond_br;
    } else if (category == XED_CATEGORY_LOGICAL) {
        target_counter = &count_logical;
    } else if (category == XED_CATEGORY_ROTATE || category == XED_CATEGORY_SHIFT) {
        target_counter = &count_rotate_shift;
    } else if (category == XED_CATEGORY_FLAGOP) {
        target_counter = &count_flag;
    } else if (category == XED_CATEGORY_AVX || category == XED_CATEGORY_AVX2 || category == XED_CATEGORY_AVX2GATHER || category == XED_CATEGORY_AVX512) {
        target_counter = &count_vector;
    } else if (category == XED_CATEGORY_CMOV) {
        target_counter = &count_cmov;
    } else if (category == XED_CATEGORY_MMX || category == XED_CATEGORY_SSE) {
        target_counter = &count_mmx_sse;
    } else if (category == XED_CATEGORY_SYSCALL) {
        target_counter = &count_syscall;
    } else if (category == XED_CATEGORY_X87_ALU) {
        target_counter = &count_fp;
    }

    // Insert Predicated Call to record Type A instruction (Always counts as 1)
    INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)FastForward, IARG_END);
    INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordInstruction, IARG_PTR, target_counter, IARG_UINT32, 1, IARG_END);
}

VOID Fini(INT32 code, VOID *v) {
    PrintStatsAndExit();
}

int main(int argc, char *argv[]) {
    if (PIN_Init(argc, argv)) {
        cerr << "Initialization failed." << endl;
        return 1;
    }

    OutFile.open(KnobOutputFile.Value().c_str());
    
    // Convert knob from billions to actual instruction count
    ff_target = KnobFastForward.Value() * 1000000000ULL;
    end_target = ff_target + 1000000000ULL;

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    return 0;
}
