#include "sysSupport.h"
#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"

extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern pcb_PTR mutexHolderProcess;
extern pcb_PTR swapMutexProcess;

/**
 * Gestisce le eccezioni a livello supporto
 */
void supportExceptionHandler() {
    // Richiedo alla SSI la struttura di supporto del current process 
    support_t *supPtr;
    ssi_payload_t payload = {
    .service_code = GETSUPPORTPTR,
    .arg = NULL,
    };
    SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &supPtr, 0);

    // Mi prendo lo stato del processore al momento dell'eccezione
    state_t *supExceptionState = &(supPtr->sup_exceptState[GENERALEXCEPT]);

    // Estraggo l'EXECCODE
    int supExceptionCause = (supExceptionState->cause & GETEXECCODE) >> CAUSESHIFT;

    // Se è uguale a 8 invoco il gestore di syscall
    if(supExceptionCause == SYSEXCEPTION) {
        supportSyscallHandler(supExceptionState);
    }
    // Altrimenti la considero come una program trap e procedo con la terminazione
    else {
        supportTrapHandler(supExceptionState);
    }

    // Incremento il PC di 4 per evitare un loop infinito
    supExceptionState->pc_epc += WORDLEN;

    // Ricarico il lo stato del processore 
    LDST(supExceptionState);
}

/**
 * @brief Gestisce le system call richieste dagli Uproc
 * 
 * @param supExceptionState Stato del processore al momento dell'eccezione
 */
void supportSyscallHandler(state_t *supExceptionState) {
    // Da reg_a0 capisco il tipo di syscall
    switch(supExceptionState->reg_a0) {
        // Se reg_a0 vale 1 allora è una sendMsg
        case SENDMSG:
            sendMsg(supExceptionState);
            break;
        // Se reg_a0 vale 2 allora è una receiveMsg
        case RECEIVEMSG:
            receiveMsg(supExceptionState);
            break;
        // per sicurezza aggiungo un ramo default che termina il processo ma non ci dovrebbe mai andare
        default:
            supportTrapHandler(supExceptionState);
            break;  
    }    
}

/**
 * USYS1: Manda un messaggio ad uno specifico processo destinatario.
 * Se a1 contiene PARENT, allora manda il messaggio al suo SST
 * 
 * @param supExceptionState Stato del processore al momento dell'eccezione
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
 * @param supExceptionState Stato del processore al momento dell'eccezione
 */
void receiveMsg(state_t *supExceptionState) {
    SYSCALL(RECEIVEMESSAGE, supExceptionState->reg_a1, supExceptionState->reg_a2, 0);
}


/**
 * Gestore delle trap a livello supporto
 * 
 * @param supExceptionState Stato del processore al momento dell'eccezione
 */
void supportTrapHandler(state_t *supExceptionState) {
    // Se il processo aveva la mutua esclusione, allora la rilascia mandando un messaggio al processo swapMutex
    if(current_process == mutexHolderProcess)
        SYSCALL(SENDMESSAGE, (unsigned int)swapMutexProcess, 0, 0);   

    ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = NULL,
    };

    // Termina il processo mandando una richiesta alla SSI
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}