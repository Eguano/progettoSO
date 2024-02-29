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
Manda un messaggio ad uno specifico processo destinatario (operazione asincrona ovvero non aspetta la receiveMessage) 
- Se avviene con successo inserisce 0 nel registro v0 del chiamante altrimenti inserisce MSGNOGOOD per segnalare errore
- Non perde il time slice rimanente
- Se il processo destinatario è nella pcbFree_h list, setta il registro v0 con DEST_NOT_EXIST
- Se il processo è nella ready queue il messaggio viene pushato nella inbox altrimenti se il processo sta aspettando un messaggio, 
deve essere svegliato e inserito nella ready queue

a0 = -1
a1 = destination process
a2 = payload

*/
void sendMessage() {
    
}


void receiveMessage() {

}


void passUpOrDie(int indexValue) {

}