#include "syscall.h"

// TODO: la systemcall SYS2 (receive) deve lasciare nel registro v0 (p_s.reg_v0)
// del ricevente il puntatore al processo mittente del msg

void syscallHandler() {

    //Incremento il PC per evitare loop infinito
    currentState->pc_epc += WORDLEN;    

    switch(currentState->reg_a0) {
        int KUp = (currentState->status >> 3) & 0x00000001;
        case SENDMESSAGE: 
            if(KUp) {
                // User mode
                PassUpOrDie(GENERALEXCEPT);
            } else {
                // Kernel mode
                sendMessage();
            }
            break;
        case RECEIVEMESSAGE:
            if(KUp) {
                // User mode
                PassUpOrDie(GENERALEXCEPT);
            } else {
                // Kernel mode
                receiveMessage();
            }
            break;
        default:
            PassUpOrDie(GENERALEXCEPT);
            break;  
    }
}

/*
    Manda un messaggio ad uno specifico processo destinatario 
*/
void sendMessage() {
    pcb_PTR destination = currentState->reg_a1;
    msg_PTR payload = currentState->reg_a2;
    int messagePushed = 0;

    // Controlla se il processo destinatario è nella pcbFree_h list
    int inPcbFree_h = 0;
    pcb_PTR iter;
    list_for_each_entry(iter, &pcbFree_h, p_list) {
        if(iter == destination) {
            inPcbFree_h = 1;
            currentState->reg_v0 = DEST_NOT_EXIST;
        }
    }

    // Controlla se il processo destinatario è nella readyQueue
    int inReadyQueue = 0;
    list_for_each_entry(iter, &readyQueue->p_list, p_list) {
        // Se il processo è nella readyQueue allora pusha il messaggio nella inbox
        if(iter == destination) {
            inReadyQueue = 1;
            pushMessage(&iter->msg_inbox, payload);
            currentState->reg_v0 = 0;
            messagePushed = 1;
        }
    }
    // Se il processo non è nella readyQueue allora inseriscilo nella readyQueue e pusha il messaggio nella inbox
    if(!inReadyQueue && !inPcbFree_h) {
        insertProcQ(&readyQueue->p_list, destination);
        pushMessage(&destination->msg_inbox, payload);
        currentState->reg_v0 = 0;
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
/*

*/
void receiveMessage() {

}


void passUpOrDie(int indexValue) {

}