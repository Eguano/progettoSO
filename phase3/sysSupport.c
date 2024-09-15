#include "sysSupport.h"
#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"

extern support_t *getSupport();
extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern pcb_PTR mutexHolderProcess;
extern pcb_PTR swapMutexProcess;

extern unsigned int debug;

/**
 * Gestisce le eccezioni a livello supporto
 */
void supportExceptionHandler() {
    debug = 0x200;
    // Richiede la struttura di supporto
    support_t *supPtr;
    ssi_payload_t payload = {
    .service_code = GETSUPPORTPTR,
    .arg = NULL,
    };
    SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &supPtr, 0);

    state_t *supExceptionState = &(supPtr->sup_exceptState[GENERALEXCEPT]);
    debug = supPtr->sup_exceptState[GENERALEXCEPT].cause;

    supExceptionState->pc_epc += WORDLEN;

    int supExceptionCause = (supExceptionState->cause & GETEXECCODE) >> CAUSESHIFT;

    if(supExceptionCause == SYSEXCEPTION) {
        debug = 0x202;
        supportSyscallHandler(supExceptionState);

    }
    else {
        debug = 0x203;
        supportTrapHandler(supExceptionState);
    }

    LDST(supExceptionState);
}

/**
 * @brief Gestisce le system call richieste dagli Uproc
 * 
 * @param supExceptionState 
 */
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
    }    
}

/**
 * USYS1: Manda un messaggio ad uno specifico processo destinatario.
 * Se a1 contiene PARENT, allora manda il messaggio al suo SST
 * 
 * @param supExceptionState 
 */
void sendMsg(state_t *supExceptionState) {
    if(supExceptionState->reg_a1 == PARENT) {
      SYSCALL(SENDMESSAGE, (unsigned int)current_process->p_parent, supExceptionState->reg_a2, 0);
    } else {
      SYSCALL(SENDMESSAGE, supExceptionState->reg_a1, supExceptionState->reg_a2, 0);
    }
}

/**
 * USYS2: Estrae un messaggio dalla inbox o, se questa è vuota, attende un messaggio
 * 
 * @param supExceptionState 
 */
void receiveMsg(state_t *supExceptionState) {
    SYSCALL(RECEIVEMESSAGE, supExceptionState->reg_a1, supExceptionState->reg_a2, 0);
}


/**
 * Gestore delle trap a livello supporto
 * 
 * @param supExceptionState 
 */
void supportTrapHandler(state_t *supExceptionState) {
    // Se il processo aveva la mutua esclusione, allora la rilascia
    if(current_process == mutexHolderProcess)
        SYSCALL(SENDMESSAGE, (unsigned int)swapMutexProcess, 0, 0);   

    ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = NULL,
    };

    // Termina il processo
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}