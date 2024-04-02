#include "syscall.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"
#include "scheduler.h"

extern pcb_PTR current_process;
extern struct list_head ready_queue;
extern struct list_head pseudoclock_blocked_list;
extern pcb_PTR ssi_pcb;
extern state_t *currentState;
extern void terminateProcess(pcb_t *proc);
extern void copyRegisters(state_t *dest, state_t *src);
extern int isInDevicesLists(pcb_t *p);

extern unsigned int debug;

/**
 * Gestisce la richiesta di una system call send o receive
 * 
 */
void syscallHandler() {
    debug = 0x600;
    int KUp = (currentState->status & USERPON);

    switch(currentState->reg_a0) {
        case SENDMESSAGE: 
            debug = 0x601;
            if(KUp) {
                // Se user mode allora setta cause.ExcCode a PRIVINSTR e invoca il gestore di Trap
                currentState->cause &= CLEAREXECCODE;
                currentState->cause |= (PRIVINSTR << CAUSESHIFT);
                passUpOrDie(GENERALEXCEPT);
            } else {
                // Se kernel mode allora invoca il metodo
                sendMessage();
                LDST(currentState);
            }
            break;
        case RECEIVEMESSAGE:
            debug = 0x602;
            if(KUp) {
                // Se user mode allora setta cause.ExcCode a PRIVINSTR e invoca il gestore di Trap
                currentState->cause &= CLEAREXECCODE;
                currentState->cause |= (PRIVINSTR << CAUSESHIFT);
                passUpOrDie(GENERALEXCEPT);
            } else {
                // Se kernel mode allora invoca il metodo
                receiveMessage();
                LDST(currentState);
            }
            break;
        default:
            debug = 0x603;
            passUpOrDie(GENERALEXCEPT);
            break;  
    }
}

/*
    Manda un messaggio ad uno specifico processo destinatario 
*/
void sendMessage() {
    debug = 0x604;
    pcb_PTR receiver = (pcb_PTR)currentState->reg_a1;
    unsigned int payload = currentState->reg_a2;
    int messagePushed = FALSE, found = FALSE;

    // Controlla se il processo destinatario è nella pcbFree_h list
    if(isInPCBFree_h(receiver)) {
        debug = 0x605;
        found = TRUE;
        currentState->reg_v0 = DEST_NOT_EXIST;
    }

    // Controlla se il processo destinatario è in esecuzione (auto-messaggio), nella readyQueue oppure nelle liste di attesa (PseudoClock o device)
    if (!found && (receiver == current_process || isInList(&ready_queue, receiver) || isInList(&pseudoclock_blocked_list, receiver) || isInDevicesLists(receiver))) {
        debug = 0x606;
        found = TRUE;
        msg_PTR toPush = createMessage(current_process, payload);
        if (toPush != NULL) {
            insertMessage(&receiver->msg_inbox, toPush);
            messagePushed = TRUE;
        }
    }

    // Il processo destinatario non è in nessuna delle precedenti liste, quindi era bloccato per la receive: viene pushato il messaggio nella inbox e poi messo in readyQueue
    if(!found) {
        debug = 0x607;
        msg_PTR toPush = createMessage(current_process, payload);
        if (toPush != NULL) {
            insertMessage(&receiver->msg_inbox, toPush);
            messagePushed = TRUE;
            insertProcQ(&ready_queue, receiver);
        }
    }

    // Se il messaggio è stato pushato correttamente nella inbox allora mettiamo in reg_v0 OK
    if(messagePushed) {
        debug = 0x608;
        currentState->reg_v0 = OK;
    } else if(!isInPCBFree_h(receiver)) {
        // Altrimenti mettiamo in reg_v0 MSGNOGOOD
        debug = 0x609;
        currentState->reg_v0 = MSGNOGOOD;
    }

    //Incremento il PC per evitare loop infinito
    currentState->pc_epc += WORDLEN;
}

/*
    Estrae un messaggio dalla inbox o, se questa è vuota, attende un messaggio
*/
void receiveMessage() {
    debug = 0x610;
    msg_PTR messageExtracted = NULL;
    // colui da cui voglio ricevere
    pcb_PTR sender = (pcb_PTR)currentState->reg_a1;
    memaddr *payload = (memaddr*) currentState->reg_a2;

    messageExtracted = popMessage(&current_process->msg_inbox, sender);

    // Il messaggio non è stato trovato (va bloccato)
    if(messageExtracted == NULL) {
        debug = 0x611;
        copyRegisters(&current_process->p_s, currentState);
        current_process->p_time += (TIMESLICE - getTIMER());
        current_process = NULL;
        schedule();
    } 
    // Il messaggio è stato trovato
    else {
        debug = 0x612;
        // Viene memorizzato il payload del messaggio nella zona puntata da reg_a2
        if(payload != NULL) {
            *payload = messageExtracted->m_payload;
        }

        // Carica in reg_v0 il processo mittente
        currentState->reg_v0 = (memaddr) messageExtracted->m_sender;

        freeMsg(messageExtracted);
        // Incremento il PC per evitare loop infinito
        currentState->pc_epc += WORDLEN;    
    }
}

/**
 * 
 * 
 * @param indexValue 
 */
void passUpOrDie(int indexValue) {
    debug = 0x613;
    
    // Pass up
    if(current_process->p_supportStruct != NULL) {
        debug = 0x614;
        copyRegisters(&current_process->p_supportStruct->sup_exceptState[indexValue], currentState);

        unsigned int stackPtr, status, progCounter;
        stackPtr = current_process->p_supportStruct->sup_exceptContext[indexValue].stackPtr;
        status = current_process->p_supportStruct->sup_exceptContext[indexValue].status;
        progCounter = current_process->p_supportStruct->sup_exceptContext[indexValue].pc;

        LDCXT(stackPtr, status, progCounter);
    }
    // Or die
    else {
        debug = 0x615;
        terminateProcess(current_process);
        current_process = NULL;
        schedule();
    }
}

/**
 * Alloca un nuovo messaggio
 * 
 * @param sender mittente del messaggio
 * @param payload puntatore al payload del messaggio
 * @return puntatore al messaggio se disponibile, NULL altrimenti
 */
msg_PTR createMessage(pcb_PTR sender, unsigned int payload) {
    msg_PTR newMsg = allocMsg();
    if (newMsg != NULL) {
        newMsg->m_sender = sender;
        newMsg->m_payload = payload;
    }
    return newMsg;
}