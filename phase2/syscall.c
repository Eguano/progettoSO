#include "syscall.h"

// TODO: la systemcall SYS2 (receive) deve lasciare nel registro v0 (p_s.reg_v0)
// del ricevente il puntatore al processo mittente del msg

void syscallHandler() {

    //Incremento il PC per evitare loop infinito
    currentState->pc_epc += WORDLEN;

    switch(currentState->reg_a0) {
        
        case SENDMESSAGE:   
            // Se il processo che ha invocato una system call era in kernel mode:
            // sendMessage()
            // Altrimenti
            // PassUpOrDie(GENERALEXCEPT)
            break;
        case RECEIVEMESSAGE:
            // Se il processo che ha invocato una system call era in kernel mode:
            // receiveMessage()
            // Altrimenti
            // PassUpOrDie(GENERALEXCEPT)
            break;
        default:
            // PassUpOrDie(GENERALEXCEPT)
            break;  
    }
}


void passUpOrDie(int indexValue) {

}


