#include "syscall.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"
#include "scheduler.h"

extern pcb_PTR current_process;
extern struct list_head ready_queue;
extern struct list_head pseudoclock_blocked_list;
extern pcb_PTR ssi_pcb;
extern pcb_PTR test_pcb;
extern pcb_PTR p2test_pcb;
extern state_t *currentState;
extern void terminateProcess(pcb_t *proc);
extern void copyRegisters(state_t *dest, state_t *src);
extern int isInDevicesLists(pcb_t *p);

extern int debug;
extern void klog_print(char *str);

/**
 * 
 * 
 */
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
    int messagePushed = FALSE, found = FALSE;

    // Controlla se il processo destinatario è nella pcbFree_h list
    debug = 616;
    if(isInPCBFree_h(receiver)) {
        debug = 617;
        found = TRUE;
        currentState->reg_v0 = DEST_NOT_EXIST;
    }

    // Controlla se il processo destinatario è in esecuzione (auto-messaggio), nella readyQueue oppure nelle liste di attesa (PseudoClock o device)
    if (!found && (receiver == current_process || isInList(&ready_queue, receiver) || isInList(&pseudoclock_blocked_list, receiver) || isInDevicesLists(receiver))) {
        debug = 618;
        found = TRUE;
        msg_PTR toPush = createMessage(current_process, payload);
        if (toPush != NULL) {
            debug = 619;
            insertMessage(&receiver->msg_inbox, toPush);
            messagePushed = TRUE;
        }
        debug = 620;
    }

    // Il processo destinatario non è in nessuna delle precedenti liste, quindi era bloccato per la receive: viene pushato il messaggio nella inbox e poi messo in readyQueue
    if(!found) {
        debug = 633;
        msg_PTR toPush = createMessage(current_process, payload);
        if (toPush != NULL) {
            insertMessage(&receiver->msg_inbox, toPush);
            messagePushed = TRUE;
            insertProcQ(&ready_queue, receiver);
        }
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
    memaddr *payload = (memaddr*) currentState->reg_a2;

    debug = 643;
    if(sender == ANYMESSAGE) {
        messageExtracted = popMessage(&current_process->msg_inbox, NULL);
    }
    else {
        messageExtracted = popMessage(&current_process->msg_inbox, sender);
    }
    debug = 644;

    // Il messaggio non è stato trovato (va bloccato)
    if(messageExtracted == NULL) {
        debug = 645;
        copyRegisters(&current_process->p_s, currentState);
        // TODO: il timer in teoria va in discesa, controllare incremento del p_time
        current_process->p_time += (TIMESLICE - getTIMER());
        current_process = NULL;
        schedule();
    } 
    // Il messaggio è stato trovato
    else {
        debug = 647;
        // Viene memorizzato il payload del messaggio nella zona puntata da reg_a2
        if(payload != 0) {
            debug = 648;
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

/**
 * 
 * 
 * @param indexValue 
 */
void passUpOrDie(int indexValue) {
    debug = 653;
    
    // Pass up
    if(current_process->p_supportStruct != NULL) {
        debug = 654;
        copyRegisters(&current_process->p_supportStruct->sup_exceptState[indexValue], currentState);

        unsigned int stackPtr, status, progCounter;
        stackPtr = current_process->p_supportStruct->sup_exceptContext[indexValue].stackPtr;
        status = current_process->p_supportStruct->sup_exceptContext[indexValue].status;
        progCounter = current_process->p_supportStruct->sup_exceptContext[indexValue].pc;

        LDCXT(stackPtr, status, progCounter);
    }
    // Or die
    else {
        terminateProcess(current_process);
        current_process = NULL;
        schedule();
    }
    debug = 656;
}

/**
 * Alloca un nuovo messaggio
 * 
 * @param sender mittente del messaggio
 * @param payload puntatore al payload del messaggio
 * @return puntatore al messaggio se disponibile, NULL altrimenti
 */
msg_PTR createMessage(pcb_PTR sender, unsigned int payload) {
    debug = 660;
    msg_PTR newMsg = allocMsg();
    if (newMsg != NULL) {
        debug = 661;
        newMsg->m_sender = sender;
        newMsg->m_payload = payload;
    }
    debug = 662;
    return newMsg;
}

/**
 * Manda un messaggio diretto all'SSI senza usare le SYSCALL.
 * Usato dall'interrupt handler per segnalare la fine di una DOIO
 * 
 * @param devStatusReg status del registro del device
 * @param toUnblock processo a cui mandare la response
 * @return stato dell'invio del messaggio
 */
int ssiDM(unsigned int devStatusReg, pcb_PTR toUnblock) {
    debug = 670;
    ssi_end_io_t endio = {
        .status = devStatusReg,
        .toUnblock = toUnblock,
    };
    ssi_payload_t payload = {
        .service_code = ENDIO,
        .arg = &endio,
    };
    
    int messagePushed = FALSE, found = FALSE, toReturn;

    // Controlla se l'ssi è nella pcbFree_h list
    if(isInPCBFree_h(ssi_pcb)) {
        debug = 671;
        found = TRUE;
        toReturn = DEST_NOT_EXIST;
    }
    debug = 672;

    // controlla se l'ssi è in esecuzione o in readyQueue
    if (!found && (ssi_pcb == current_process || isInList(&ready_queue, ssi_pcb))) {
        debug = 673;
        found = TRUE;
        msg_PTR toPush = createMessage(NULL, (unsigned int) &payload);
        if (toPush != NULL) {
            insertMessage(&ssi_pcb->msg_inbox, toPush);
            messagePushed = TRUE;
        }
    }
    debug = 674;

    // L'ssi non è in nessuna delle precedenti liste, quindi era bloccato per la receive
    if(!found) {
        debug = 677;
        msg_PTR toPush = createMessage(NULL, (unsigned int) &payload);
        if (toPush != NULL) {
            insertMessage(&ssi_pcb->msg_inbox, toPush);
            messagePushed = TRUE;
            insertProcQ(&ready_queue, ssi_pcb);
        }
    }
    debug = 678;

    // Se il messaggio è stato pushato correttamente nella inbox allora mettiamo in reg_v0 OK
    if(messagePushed) {
        debug = 679;
        toReturn = OK;
    } else if(!isInPCBFree_h(ssi_pcb)) {
        // Altrimenti mettiamo in reg_v0 MSGNOGOOD
        debug = 680;
        toReturn = MSGNOGOOD;
    }
    debug = 681;

    return toReturn;
}