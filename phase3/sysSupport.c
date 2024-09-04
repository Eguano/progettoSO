#include "sysSupport.h"
#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"

extern support_t *getSupport();
extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern pcb_PTR mutexHolderProcess;
extern pcb_PTR swapMutexProcess;

extern unsigned int debug;

void supportExceptionHandler() {
    debug = 0x200;
    support_t *supPtr = getSupport();
    debug = 0x201;
    state_t *supExceptionState = &(supPtr->sup_exceptState[GENERALEXCEPT]);
    unsigned int supExceptionCause = (supExceptionState->cause & GETEXECCODE) >> CAUSESHIFT;
    
    if(supExceptionCause == SYSEXCEPTION) {
        debug = 0x202;
        supportSyscallHandler(supExceptionState);
    }
    else {
        debug = 0x203;
        supportTrapHandler(supExceptionState);
    }
}

void supportSyscallHandler(state_t *supExceptionState) {
    switch(supExceptionState->reg_a0) {
        case SENDMSG:
            sendMsg(supExceptionState);
            break;
        case RECEIVEMSG:
            receiveMsg(supExceptionState);
            break;
        default:
            supportTrapHandler(supExceptionState);
            break;  
        
        supExceptionState->pc_epc += WORDLEN;
        LDST(supExceptionState);
    }
}

void sendMsg(state_t *supExceptionState) {
    if (supExceptionState->reg_a1 == PARENT) {
      SYSCALL(SENDMESSAGE, (unsigned int)current_process->p_parent, supExceptionState->reg_a2, 0);
    } else {
      SYSCALL(SENDMESSAGE, supExceptionState->reg_a1, supExceptionState->reg_a2, 0);
    }
}

void receiveMsg(state_t *supExceptionState) {
    SYSCALL(RECEIVEMESSAGE, supExceptionState->reg_a1, supExceptionState->reg_a2, 0);
}

void supportTrapHandler(state_t *supExceptionState) {
    if (current_process == mutexHolderProcess)
        SYSCALL(SENDMESSAGE, (unsigned int)swapMutexProcess, 0, 0);   

    ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = NULL,
    };

    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}