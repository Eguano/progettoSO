#include "syscall.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"

extern state_t *currentState;
extern pcb_PTR ready_queue;
extern pcb_PTR current_process;

extern void schedule();

// TODO: SPOSTARE RICERCA DEL PCB NELLA FREELIST

void syscallHandler() {

    //Incremento il PC per evitare loop infinito
    currentState->pc_epc += WORDLEN;    

    int KUp = (currentState->status >> 3) & 0x00000001;

    switch(currentState->reg_a0) {
        case SENDMESSAGE: 
            if(KUp) {
                // User mode
                passUpOrDie(GENERALEXCEPT);
            } else {
                // Kernel mode
                sendMessage();
            }
            break;
        case RECEIVEMESSAGE:
            if(KUp) {
                // User mode
                passUpOrDie(GENERALEXCEPT);
            } else {
                // Kernel mode
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
    int messagePushed = 0;

    // Controlla se il processo destinatario è nella pcbFree_h list
    int inPcbFree_h = isInPCBFree_h(receiver);
    if(inPcbFree_h) {
        currentState->reg_v0 = DEST_NOT_EXIST;
    }    

    // Controlla se il processo destinatario è nella readyQueue
    int inReadyQueue = 0;
    pcb_PTR iter;
    list_for_each_entry(iter, &ready_queue->p_list, p_list) {
        // Se il processo è nella readyQueue allora pusha il messaggio nella inbox
        if(iter == receiver) {
            inReadyQueue = 1;
            pushMessage(&iter->msg_inbox, payload);
            currentState->reg_v0 = OK;
            messagePushed = 1;
        }
    }
    // Se il processo non è nella readyQueue allora inseriscilo nella readyQueue e pusha il messaggio nella inbox
    if(!inReadyQueue && !inPcbFree_h) {
        // 
        insertProcQ(&ready_queue->p_list, receiver);
        pushMessage(&receiver->msg_inbox, payload);
        currentState->reg_v0 = OK;
        messagePushed = 1;
    }

    // Altrimenti ritorna MSGNOGOOD
    if(!messagePushed && !inPcbFree_h) {
        currentState->reg_v0 = MSGNOGOOD;
    }
}


/*
    This system call is used by a process to extract a message from its inbox or, if this one is empty, to wait for a message
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
        // Rimuoviamo il processo dalla ready queue
        list_del(&current_process->p_list);
        // Aggiungere il processo nella lista di processi bloccati (?)
        current_process->p_s = *currentState;
        current_process->p_time += getTIMER();
        schedule();
    } 
    // Il messaggio è stato trovato
    else {
        // Memorizzare il payload del messaggio nella zona puntata da reg_a2
        if(payload != (memaddr) NULL) {
            payload = messageExtracted->m_payload;
        }

        // Carica in reg_v0 il processo mittente
        currentState->reg_v0 = (memaddr) messageExtracted->m_sender;

        freeMsg(messageExtracted);
    }
}


void passUpOrDie(int indexValue) {

}