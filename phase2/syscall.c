#include "syscall.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"

extern state_t *currentState;
extern struct list_head ready_queue;
extern pcb_PTR current_process;
extern struct list_head pseudoclock_blocked_list;

extern void schedule();
extern void terminateProcess(pcb_t *proc);

extern int debug;
extern void klog_print(char *str);

void syscallHandler() {
    debug = 600;

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
                LDST(currentState);
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
                LDST(currentState);
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
    int messagePushed = FALSE, found = FALSE;

    // Controlla se il processo destinatario è nella pcbFree_h list
    debug = 616;
    if(isInPCBFree_h(receiver)) {
        debug = 617;
        found = TRUE;
        currentState->reg_v0 = DEST_NOT_EXIST;
    }

    // controlla se il processo destinatario è quello in esecuzione (auto-messaggio)
    if (!found && receiver == current_process) {
        debug = 618;
        found = TRUE;
        msg_PTR toPush = createMessage(current_process, payload);
        pushMessage(&receiver->msg_inbox, toPush);
        messagePushed = TRUE;
    }

    // Controlla se il processo destinatario è nella readyQueue
    if(!found) {
        debug = 619;
        list_for_each_entry(iter, &ready_queue, p_list) {
            debug = 620;
            // Se il processo è nella readyQueue allora viene pushato il messaggio nella inbox
            if(iter == receiver) {
                debug = 621;
                found = TRUE;
                msg_PTR toPush = createMessage(current_process, payload);
                pushMessage(&receiver->msg_inbox, toPush);
                debug = 622;
                messagePushed = TRUE;
            }
            debug = 623;
        }
        debug = 624;
    }
    debug = 625;

    // Controlla se il processo destinatario è nella pseudoclockBlocked
    if(!found) {
        debug = 626;
        list_for_each_entry(iter, &pseudoclock_blocked_list, p_list) {
            debug = 627;
            // Se il processo è nella pseudoclockBlocked allora viene pushato il messaggio nella inbox senza metterlo in readyQueue
            if(iter == receiver) {
                debug = 628;
                found = TRUE;
                msg_PTR toPush = createMessage(current_process, payload);
                pushMessage(&receiver->msg_inbox, toPush);
                messagePushed = TRUE;
                debug = 629;
            }
            debug = 630;
        }
        debug = 631;
    }
    debug = 632;

    // TODO: perchè non controlla le liste dei device terminali ed esterni?

    // Il processo destinatario non è in nessuna delle precedenti liste, quindi era bloccato per la receive: viene pushato il messaggio nella inbox e poi messo in readyQueue
    if(!found) {
        debug = 633;
        msg_PTR toPush = createMessage(current_process, payload);
        pushMessage(&receiver->msg_inbox, toPush);
        messagePushed = TRUE;
        insertProcQ(&ready_queue, receiver);
        debug = 634;
    }
    debug = 635;

    // Se il messaggio è stato pushato correttamente nella inbox allora mettiamo in reg_v0 OK
    if(messagePushed) {
        debug = 636;
        currentState->reg_v0 = OK;
    } else if(!isInPCBFree_h(receiver)) {
        // Altrimenti mettiamo in reg_v0 MSGNOGOOD
        debug = 638;
        currentState->reg_v0 = MSGNOGOOD;
    }
    debug = 639;

    //Incremento il PC per evitare loop infinito
    currentState->pc_epc += WORDLEN;
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
    memaddr *payload = (memaddr*) currentState->reg_a2;

    debug = 643;
    messageExtracted = popMessage(&current_process->msg_inbox, sender);
    debug = 644;

    // Il messaggio non è stato trovato (va bloccato)
    if(messageExtracted == NULL) {
        debug = 645;
        // TODO: verificare se funziona davvero
        STST(&current_process->p_s);
        current_process->p_s.pc_epc = currentState->pc_epc;
        current_process->p_s.reg_a0 = RECEIVEMESSAGE;
        current_process->p_s.reg_a1 = (unsigned int) sender;
        current_process->p_s.reg_a2 = (unsigned int) payload;
        // TODO: il timer in teoria va in discesa, controllare incremento del p_time
        current_process->p_time += getTIMER();
        // TODO: current_process = NULL; ??? serve forse
        schedule();
        debug = 646;
    } 
    // Il messaggio è stato trovato
    else {
        debug = 647;
        // Viene memorizzato il payload del messaggio nella zona puntata da reg_a2
        if(payload != NULL) {
            debug = 648;
            // TODO: controllare memorizzazione del payload
            *payload = messageExtracted->m_payload;
        }
        debug = 649;

        // Carica in reg_v0 il processo mittente
        currentState->reg_v0 = (memaddr) messageExtracted->m_sender;

        debug = 650;
        freeMsg(messageExtracted);
        // Incremento il PC per evitare loop infinito
        currentState->pc_epc += WORDLEN;    
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
        terminateProcess(current_process);
        schedule();
    }
    debug = 656;
}

msg_PTR createMessage(pcb_PTR sender, unsigned int payload) {
    msg_PTR newMsg = allocMsg();
    if (newMsg == NULL) {
      // TODO: non ci sono più messaggi disponibili, che si fa?
    } else {
        newMsg->m_sender = sender;
        newMsg->m_payload = payload;
    }
    return newMsg;
}