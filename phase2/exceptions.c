#include "exceptions.h"

#include "syscall.h"

extern state_t *currentState;
extern pcb_PTR current_process;
extern void interruptHandler();

/**
 * Gestisce il caso in cui si prova accedere ad un indirizzo 
 * virtuale che non ha un corrispettivo nella TLB
 */
void uTLB_RefillHandler() {
    // Prendo il page number da entryHi
    unsigned int p = (currentState->entry_hi >> 12) & VPNMASK;

    // Mi preparo per inserire la pagina nel TLB
    setENTRYHI(current_process->p_supportStruct->sup_privatePgTbl[p].pte_entryHI);
    setENTRYLO(current_process->p_supportStruct->sup_privatePgTbl[p].pte_entryLO);

    // La inserisco
    TLBWR();

    // Restituisco il controllo
    LDST(currentState);
}

/**
 * Gestisce tutti gli altri tipi di eccezione
 */
void exceptionHandler() {
    switch((getCAUSE() & GETEXECCODE) >> CAUSESHIFT) {
        case IOINTERRUPTS:
            // External Device Interrupt
            interruptHandler();
            break;
        case 1 ... 3:
            // TLB Exception
            // uTLB_RefillHandler();
            passUpOrDie(PGFAULTEXCEPT);
            break;
        case 4 ... 7:
            // Program Traps p1: Address and Bus Error Exception
            passUpOrDie(GENERALEXCEPT);
            break;
        case SYSEXCEPTION: 
            // Syscalls
            syscallHandler();
		    break;
        case 9 ... 12:
            // Breakpoint Calls, Program Traps p2
            passUpOrDie(GENERALEXCEPT);
            break;
        default: 
            // Wrong ExcCode
            passUpOrDie(GENERALEXCEPT);
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