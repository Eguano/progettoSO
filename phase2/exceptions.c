#include "exceptions.h"

state_t *currentState = (state_t *)BIOSDATAPAGE;    

/**
 * Gestisce il caso in cui si prova accedere ad un indirizzo 
 * virtuale che non ha un corrispettivo nella TLB
 */
void uTLB_RefillHandler() {
    setENTRYHI(0x80000000);
    setENTRYLO(0x00000000);
    TLBWR();
    LDST((state_t*) 0x0FFFF000);
}


/**
 * Gestisce tutte gli altri tipi di eccezioni
 * 
 */
void exceptionHandler() {
    switch((currentState->cause & GETEXECCODE >> 2)) {
        case 0:
            // External Device Interrupt
            break;
        case 1 ... 3:
            // TLB Exception
            break;
        case 4 ... 7:
            // Program Traps p1: Address and Bus Error Exception
            break;
        case 8: 
            syscallHandler();
		    break;
        case 9 ... 12:
            // Breakpoint Calls, Program Traps p2
        default: 
            // Wrong ExcCode
            break;
	}
}



/*
    Number - Code - Description
    0 - Int - External Device Interrupt
    1 - Mod - TLB-Modification Exception
    2 - TLBL - TLB Invalid Exception: on a Load instr. or instruction fetch
    3 - TLBS - TLB Invalid Exception: on a Store instr.
    4 - AdEL - Address Error Exception: on a Load or instruction fetch
    5 - AdES - Address Error Exception: on a Store instr.
    6 - IBE - Bus Error Exception: on an instruction fetch
    7 - DBE - Bus Error Exception: on a Load/Store data access
    8 - Sys - Syscall Exception
    9 - Bp - Breakpoint Exception
    10 - RI - Reserved Instruction Exception
    11 - CpU - Coprocessor Unusable Exception
    12 - OV - Arithmetic Overflow Exception
*/