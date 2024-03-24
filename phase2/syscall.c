#include "syscall.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"

extern state_t *currentState;
extern struct list_head ready_queue;
extern pcb_PTR current_process;
extern struct list_head pseudoclock_blocked_list;
extern int debug;

extern void schedule();
extern void SSIRequest(pcb_t* sender, int service, void* arg);

// TODO: SPOSTARE RICERCA DEL PCB NELLA FREELIST

void syscallHandler() {
    debug = 600;

    //Incremento il PC per evitare loop infinito
    currentState->pc_epc += WORDLEN;    

    int KUp = (currentState->status & USERPON);
    debug = 601;

    switch(currentState->reg_a0) {
        case SENDMESSAGE: 
            debug = 602;
            if(KUp) {
                debug = 603;
                // Se user mode allora setta cause.ExcCode a PRIVINSTR e invoca il gestore di Trap
                currentState->cause &= CLEAREXECCODE;
                currentState->cause |= (PRIVINSTR << CAUSESHIFT);
                passUpOrDie(GENERALEXCEPT);
                debug = 604;
            } else {
                debug = 605;
                // Se kernel mode allora invoca il metodo
                sendMessage();
                debug = 606;
            }
            break;
        case RECEIVEMESSAGE:
            debug = 607;
            if(KUp) {
                debug = 608;
                // Se user mode allora setta cause.ExcCode a PRIVINSTR e invoca il gestore di Trap
                currentState->cause &= CLEAREXECCODE;
                currentState->cause |= (PRIVINSTR << CAUSESHIFT);
                passUpOrDie(GENERALEXCEPT);
                debug = 609;
            } else {
                debug = 610;
                // Se kernel mode allora invoca il metodo
                receiveMessage();
                debug = 611;
            }
            break;
        default:
            debug = 612;
            passUpOrDie(GENERALEXCEPT);
            debug = 613;
            break;  
    }
    debug = 614;
}

/*
    Manda un messaggio ad uno specifico processo destinatario 
*/
void sendMessage() {
    debug = 615;
    pcb_PTR receiver = (pcb_PTR)currentState->reg_a1;
    msg_PTR payload = (msg_PTR)currentState->reg_a2;
    pcb_PTR iter;
    int messagePushed = 0;

    // Controlla se il processo destinatario è nella pcbFree_h list
    int inPcbFree_h = isInPCBFree_h(receiver);
    debug = 616;
    if(inPcbFree_h) {
        debug = 617;
        currentState->reg_v0 = DEST_NOT_EXIST;
    }
    debug = 618;

    // Controlla se il processo destinatario è nella readyQueue
    int inReadyQueue = 0;
    if(!inPcbFree_h) {
        debug = 619;
        list_for_each_entry(iter, &ready_queue, p_list) {
            debug = 620;
            // Se il processo è nella readyQueue allora viene pushato il messaggio nella inbox
            if(iter == receiver) {
                debug = 621;
                inReadyQueue = 1;
                pushMessage(&receiver->msg_inbox, payload);
                debug = 622;
                messagePushed = 1;
            }
            debug = 623;
        }
        debug = 624;
    }
    debug = 625;

    // Controlla se il processo destinatario è nella pseudoclockBlocked
    int inPseudoClock = 0;
    if(!inReadyQueue && !inPcbFree_h) {
        debug = 626;
        list_for_each_entry(iter, &pseudoclock_blocked_list, p_list) {
            debug = 627;
            // Se il processo è nella pseudoclockBlocked allora viene pushato il messaggio nella inbox senza metterlo in readyQueue
            if(iter == receiver) {
                debug = 628;
                inPseudoClock = 1;
                pushMessage(&receiver->msg_inbox, payload);
                messagePushed = 1;
                debug = 629;
            }
            debug = 630;
        }
        debug = 631;
    }
    debug = 632;

    // Il processo destinatario non è in nessuna delle precedenti liste, quindi viene messo in readyQueue e poi pushato il messaggio nella inbox
    if(!inReadyQueue && !inPcbFree_h && !inPseudoClock) {
        debug = 633;
        insertProcQ(&ready_queue, receiver);
        pushMessage(&receiver->msg_inbox, payload);
        messagePushed = 1;
        debug = 634;
    }
    debug = 635;

    // Se il messaggio è stato pushato correttamente nella inbox allora mettiamo in reg_v0 OK
    if(messagePushed) {
        debug = 636;
        currentState->reg_v0 = OK;
    }
    debug = 637;
    // Altrimenti mettiamo in reg_v0 MSGNOGOOD
    if(!messagePushed && !inPcbFree_h) {
        debug = 638;
        currentState->reg_v0 = MSGNOGOOD;
    }
    debug = 639;
}


/*
    Estrae un messaggio dalla inbox o, se questa è vuota, attende un messaggio
*/
void receiveMessage() {
    debug = 640;
    msg_PTR messageExtracted = NULL;
    pcb_PTR sender = (pcb_PTR)currentState->reg_a1;
    unsigned int payload = currentState->reg_a2;

    debug = 641;
    if(sender == ANYMESSAGE) {
        debug = 642;
        sender = NULL;
    }
    debug = 643;
    messageExtracted = popMessage(&current_process->msg_inbox, sender);
    debug = 644;

    // Il messaggio non è stato trovato (va bloccato)
    if(messageExtracted == NULL) {
        debug = 645;
        current_process->p_s = *currentState;
        current_process->p_time += getTIMER();
        schedule();
        debug = 646;
    } 
    // Il messaggio è stato trovato
    else {
        debug = 647;
        // Viene memorizzato il payload del messaggio nella zona puntata da reg_a2
        if(payload != (memaddr) NULL) {
            debug = 648;
            payload = messageExtracted->m_payload;
        }
        debug = 649;

        // Carica in reg_v0 il processo mittente
        currentState->reg_v0 = (memaddr) messageExtracted->m_sender;

        debug = 650;
        freeMsg(messageExtracted);
        debug = 651;
    }
    debug = 652;
  
}


void passUpOrDie(int indexValue) {
    debug = 653;
    
    // Pass up
    if(current_process->p_supportStruct != NULL) {
        debug = 654;
        current_process->p_supportStruct->sup_exceptState[indexValue] = *currentState;

        unsigned int stackPtr, status, progCounter;
        stackPtr = current_process->p_supportStruct->sup_exceptContext[indexValue].stackPtr;
        status = current_process->p_supportStruct->sup_exceptContext[indexValue].status;
        progCounter = current_process->p_supportStruct->sup_exceptContext[indexValue].pc;

        LDCXT(stackPtr, status, progCounter);
    }
    // Or die
    else {
        debug = 655;
        SSIRequest(current_process, TERMPROCESS, NULL);
    }
    debug = 656;
}