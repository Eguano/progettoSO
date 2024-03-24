#include "syscall.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"

extern state_t *currentState;
extern struct list_head ready_queue;
extern pcb_PTR current_process;
extern struct list_head pseudoclock_blocked_list;

extern void schedule();
extern void SSIRequest(pcb_t* sender, int service, void* arg);

// TODO: SPOSTARE RICERCA DEL PCB NELLA FREELIST

void syscallHandler() {

    //Incremento il PC per evitare loop infinito
    currentState->pc_epc += WORDLEN;    

    int KUp = (currentState->status & USERPON);

    switch(currentState->reg_a0) {
        case SENDMESSAGE: 
            if(KUp) {
                // Se user mode allora setta cause.ExcCode a PRIVINSTR e invoca il gestore di Trap
                currentState->cause &= CLEAREXECCODE;
                currentState->cause |= (PRIVINSTR << CAUSESHIFT);
                passUpOrDie(GENERALEXCEPT);
            } else {
                // Se kernel mode allora invoca il metodo
                sendMessage();
            }
            break;
        case RECEIVEMESSAGE:
            if(KUp) {
                // Se user mode allora setta cause.ExcCode a PRIVINSTR e invoca il gestore di Trap
                currentState->cause &= CLEAREXECCODE;
                currentState->cause |= (PRIVINSTR << CAUSESHIFT);
                passUpOrDie(GENERALEXCEPT);
            } else {
                // Se kernel mode allora invoca il metodo
                receiveMessage();
            }
            break;
        default:
            passUpOrDie(GENERALEXCEPT);
            break;  
    }
}

/*
    Manda un messaggio ad uno specifico processo destinatario 
*/
void sendMessage() {
    pcb_PTR receiver = (pcb_PTR)currentState->reg_a1;
    msg_PTR payload = (msg_PTR)currentState->reg_a2;
    pcb_PTR iter;
    int messagePushed = 0;

    // Controlla se il processo destinatario è nella pcbFree_h list
    int inPcbFree_h = isInPCBFree_h(receiver);
    if(inPcbFree_h) {
        currentState->reg_v0 = DEST_NOT_EXIST;
    }  

    // Controlla se il processo destinatario è nella readyQueue
    int inReadyQueue = 0;
    if(!inPcbFree_h) {
        list_for_each_entry(iter, &ready_queue, p_list) {
            // Se il processo è nella readyQueue allora viene pushato il messaggio nella inbox
            if(iter == receiver) {
                inReadyQueue = 1;
                pushMessage(&receiver->msg_inbox, payload);
                messagePushed = 1;
            }
        }
    }

    // Controlla se il processo destinatario è nella pseudoclockBlocked
    int inPseudoClock = 0;
    if(!inReadyQueue && !inPcbFree_h) {
        list_for_each_entry(iter, &pseudoclock_blocked_list, p_list) {
            // Se il processo è nella pseudoclockBlocked allora viene pushato il messaggio nella inbox senza metterlo in readyQueue
            if(iter == receiver) {
                inPseudoClock = 1;
                pushMessage(&receiver->msg_inbox, payload);
                messagePushed = 1;
            }
        }
    }

    // Il processo destinatario non è in nessuna delle precedenti liste, quindi viene messo in readyQueue e poi pushato il messaggio nella inbox
    if(!inReadyQueue && !inPcbFree_h && !inPseudoClock) {
        insertProcQ(&ready_queue, receiver);
        pushMessage(&receiver->msg_inbox, payload);
        messagePushed = 1;
    }

    // Se il messaggio è stato pushato correttamente nella inbox allora mettiamo in reg_v0 OK
    if(messagePushed) {
        currentState->reg_v0 = OK;
    }
    // Altrimenti mettiamo in reg_v0 MSGNOGOOD
    if(!messagePushed && !inPcbFree_h) {
        currentState->reg_v0 = MSGNOGOOD;
    }
}


/*
    Estrae un messaggio dalla inbox o, se questa è vuota, attende un messaggio
*/
void receiveMessage() {
    msg_PTR messageExtracted = NULL;
    pcb_PTR sender = (pcb_PTR)currentState->reg_a1;
    unsigned int payload = currentState->reg_a2;

    if(sender == ANYMESSAGE) {
        sender = NULL;
    }
    messageExtracted = popMessage(&current_process->msg_inbox, sender);

    // Il messaggio non è stato trovato (va bloccato)
    if(messageExtracted == NULL) {
        current_process->p_s = *currentState;
        current_process->p_time += getTIMER();
        schedule();
    } 
    // Il messaggio è stato trovato
    else {
        // Viene memorizzato il payload del messaggio nella zona puntata da reg_a2
        if(payload != (memaddr) NULL) {
            payload = messageExtracted->m_payload;
        }

        // Carica in reg_v0 il processo mittente
        currentState->reg_v0 = (memaddr) messageExtracted->m_sender;

        freeMsg(messageExtracted);
    }
  
}


void passUpOrDie(int indexValue) {
    
    // Pass up
    if(current_process->p_supportStruct != NULL) {
        current_process->p_supportStruct->sup_exceptState[indexValue] = *currentState;

        unsigned int stackPtr, status, progCounter;
        stackPtr = current_process->p_supportStruct->sup_exceptContext[indexValue].stackPtr;
        status = current_process->p_supportStruct->sup_exceptContext[indexValue].status;
        progCounter = current_process->p_supportStruct->sup_exceptContext[indexValue].pc;

        LDCXT(stackPtr, status, progCounter);
    }
    // Or die
    else {
        SSIRequest(current_process, TERMPROCESS, NULL);
    }
}