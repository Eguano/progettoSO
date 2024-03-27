#include "syscall.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"

extern state_t *currentState;
extern struct list_head ready_queue;
extern pcb_PTR current_process;
extern struct list_head pseudoclock_blocked_list;
extern int debug;
unsigned int debugMsg;
unsigned int debugMsg2;

extern void schedule();

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
    unsigned int payload = currentState->reg_a2;
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

    // controlla se il processo destinatario è quello in esecuzione (auto-messaggio)
    int isCurrent = 0;
    if (!inPcbFree_h && receiver == current_process) {
        isCurrent = 1;
        msg_t toPush = {
            .m_sender = current_process,
            .m_payload = payload,
        };
        pushMessage(&receiver->msg_inbox, &toPush);
        messagePushed = 1;
    }

    // Controlla se il processo destinatario è nella readyQueue
    int inReadyQueue = 0;
    if(!inPcbFree_h && !isCurrent) {
        debug = 619;
        list_for_each_entry(iter, &ready_queue, p_list) {
            debug = 620;
            // Se il processo è nella readyQueue allora viene pushato il messaggio nella inbox
            if(iter == receiver) {
                debug = 621;
                inReadyQueue = 1;
                msg_t toPush = {
                    .m_sender = current_process,
                    .m_payload = payload,
                };
                pushMessage(&receiver->msg_inbox, &toPush);
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
    if(!inReadyQueue && !inPcbFree_h && !isCurrent) {
        debug = 626;
        list_for_each_entry(iter, &pseudoclock_blocked_list, p_list) {
            debug = 627;
            // Se il processo è nella pseudoclockBlocked allora viene pushato il messaggio nella inbox senza metterlo in readyQueue
            if(iter == receiver) {
                debug = 628;
                inPseudoClock = 1;
                msg_t toPush = {
                    .m_sender = current_process,
                    .m_payload = payload,
                };
                pushMessage(&receiver->msg_inbox, &toPush);
                messagePushed = 1;
                debug = 629;
            }
            debug = 630;
        }
        debug = 631;
    }
    debug = 632;

    // TODO: perchè non controlla le liste dei device terminali ed esterni?

    // Il processo destinatario non è in nessuna delle precedenti liste, quindi era stato sbloccato dalle liste di attesa per device: viene messo in readyQueue e poi pushato il messaggio nella inbox
    if(!inReadyQueue && !inPcbFree_h && !inPseudoClock && !isCurrent) {
        debug = 633;
        msg_t toPush = {
            .m_sender = current_process,
            .m_payload = payload,
        };
        pushMessage(&receiver->msg_inbox, &toPush);
        messagePushed = 1;
        insertProcQ(&ready_queue, receiver);
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

    debugMsg2 = (memaddr) headMessage(&current_process->msg_inbox);

    LDST(currentState);
}


/*
    Estrae un messaggio dalla inbox o, se questa è vuota, attende un messaggio
*/
void receiveMessage() {
    debug = 640;
    msg_PTR messageExtracted = NULL;
    // colui da cui voglio ricevere
    pcb_PTR sender = (pcb_PTR)currentState->reg_a1;
    // TODO: payload dovrebbe essere un puntatore ad unsigned int ?
    unsigned int payload = currentState->reg_a2;

    debug = 643;
    messageExtracted = popMessage(&current_process->msg_inbox, sender);
    debug = 644;

    // Il messaggio non è stato trovato (va bloccato)
    if(messageExtracted == NULL) {
        debug = 645;
        // TODO: usare STST? da libumps.h
        current_process->p_s = *currentState;
	    current_process->p_time += (TIMESLICE - getTIMER());
        schedule();
        debug = 646;
    } 
    // Il messaggio è stato trovato
    else {
        debug = 647;
        // Viene memorizzato il payload del messaggio nella zona puntata da reg_a2
        if(payload != (memaddr) NULL) {
            debug = 648;
            // TODO: controllare memorizzazione del payload
            payload = messageExtracted->m_payload;
        }
        debug = 649;

        // Carica in reg_v0 il processo mittente
        currentState->reg_v0 = (memaddr) messageExtracted->m_sender;

        debugMsg = (memaddr) messageExtracted;

        debug = 650;
        // freeMsg(messageExtracted);
        debug = 651;
    }
    debug = 652;
    
    LDST(currentState);
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
        // TODO: chiedere all'ssi la terminazione oppure usare le sue funzioni per terminare qui (seconda opzione preferibile)
    }
    debug = 656;
}
