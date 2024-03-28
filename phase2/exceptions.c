#include "exceptions.h"

#include "syscall.h"

extern state_t *currentState;
extern void interruptHandler();

extern int debug;

/**
 * Gestisce il caso in cui si prova accedere ad un indirizzo 
 * virtuale che non ha un corrispettivo nella TLB
 */
void uTLB_RefillHandler() {
    debug = 100;
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
    debug = 102;
    switch((getCAUSE() & GETEXECCODE) >> CAUSESHIFT) {
        case IOINTERRUPTS:
            // External Device Interrupt
            interruptHandler();
            break;
        case 1 ... 3:
            // TLB Exception
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
    debug = 103;
}

/*
    Funzione per copiare strutture
    TODO: togliere quando non servirà più negli interrupt
*/
void memcpy(memaddr *src, memaddr *dest, unsigned int bytes) {
    debug = 104;
    for(int i = 0; i < (bytes/4); i++){
        *dest = *src;
        dest++;
        src++;
    }
    debug = 105;
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