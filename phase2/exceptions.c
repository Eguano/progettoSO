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
    switch(currentState->cause) {
        // IO Interrupt (0-7 interruput lines)
        // Syscalls
        // Breakpoint calls
        // Page faults
        // Program traps (address error, bus error, reserved instruction, coprocessor unusable, arithmetic overflow)
    }
    
}