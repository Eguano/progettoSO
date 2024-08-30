#include "interrupt.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"
#include "scheduler.h"

extern int waiting_count;
extern pcb_PTR current_process;
extern struct list_head ready_queue;
extern struct list_head external_blocked_list[4][MAXDEV];
extern struct list_head pseudoclock_blocked_list;
extern struct list_head terminal_blocked_list[2][MAXDEV];
extern pcb_PTR ssi_pcb;
extern state_t *currentState;
extern void copyRegisters(state_t *dest, state_t *src);
extern msg_PTR createMessage(pcb_PTR sender, unsigned int payload);

// payload for direct message to SSI
ssi_payload_t payloadDM = {
    .service_code = ENDIO,
    .arg = NULL,
};

/**
 * Gestisce tutti i tipi di interrupt
 */
void interruptHandler() {
    unsigned int causeReg = getCAUSE();
    // Gli interrupt sono abilitati a livello globale
    if(((currentState->status & IEPON) >> 2) == 1) {

        for(int line = 0; line < 8; line++) {
            // Se la line e' attiva
            if(intPendingInLine(causeReg, line)) {
                switch(line) {
                    case 0:
                    break;
                    case 1:
                        if(intLineActive(1)) {
                            // scaduto il timeslice del running process
                            PLTInterruptHandler();
                        }
                    break;
                    case 2:
                        if(intLineActive(2)) {
                            // scattato lo pseudoclock (interval timer)
                            ITInterruptHandler();
                        }   
                    break;
                    case 5:
                    break;
                    default:
                        // Controllo se la interrupt line corrente e' attiva facendo & e shiftando a sx di 8 + line
                        if(intLineActive(line)) {

                            // Punto alla interrupt line corrente usando la interrupt devices bit map
                            unsigned int *intLaneMapped = (memaddr *)(INTDEVBITMAP + (0x4 * (line - 3)));

                            for(int dev = 0; dev < 8; dev++) {
                                if(intPendingOnDev(intLaneMapped, dev)) {
                                    
                                    pcb_PTR toUnblock;
                                    unsigned int devStatusReg;

                                    // Sto trattando un terminal device interrupt   
                                    if(line == 7) {
                                        toUnblock = termDevInterruptHandler(&devStatusReg, line, dev);
                                    }
                                    // Sto trattando un external device interrupt
                                    else {
                                        toUnblock = extDevInterruptHandler(&devStatusReg, line, dev);
                                    }
                                    
                                    // controllo se c'è un processo da sbloccare
                                    if(toUnblock != NULL) {
                                        waiting_count--;
                                        // Salvo lo status register su v0 del processo da sbloccare
                                        toUnblock->p_s.reg_v0 = devStatusReg;

                                        // mando un messaggio diretto al ssi per sbloccare il processo
                                        msg_PTR toPush = createMessage(toUnblock, (unsigned int) &payloadDM);
                                        if (toPush != NULL) {
                                            insertMessage(&ssi_pcb->msg_inbox, toPush);
                                            // controllo se l'ssi è in esecuzione o in readyQueue
                                            if (ssi_pcb != current_process && !isInList(&ready_queue, ssi_pcb)) {
                                                insertProcQ(&ready_queue, ssi_pcb);
                                            }
                                        } 
                                    }

                                    if(current_process == NULL)
                                        schedule();
                                    else
                                        LDST(currentState);
                                }
                            }
                        }
                    break;
                }
            }
        }
    }
}

/**
 * Data l'interrupt line e la device interrupt line, calcola l'indirizzo base del device register.
 * 
 * @param intLine 
 * @param devIntLine 
 * @return 
 */
memaddr *getDevReg(unsigned int intLine, unsigned int devIntLine) {
    return GET_DEV_REG(intLine, devIntLine);
}

/**
 * Controlla se una specifica interrupt line e' attiva o mascherata.
 * 
 * @param line 
 * @return 
 */
unsigned short int intLineActive(unsigned short int line) {
    return (((currentState->status & IMON) >> (8 + line)) & 0x1) == 1; 
}

/**
 * Controlla se in una determinata interrupt line e' presente un interrupt.
 * 
 * @param causeReg 
 * @param line 
 * @return 
 */
unsigned short int intPendingInLine(unsigned int causeReg, unsigned short int line) {
    return (((causeReg & interruptConsts[line]) >> (8 + line))) == 1;
}

/**
 * Dato un device, controlla se esso ha alzato o meno un interrupt.
 * 
 * @param intLaneMapped 
 * @param dev 
 * @return 
 */
unsigned short int intPendingOnDev(unsigned int *intLaneMapped, unsigned int dev) {
    return (((*intLaneMapped) & deviceConsts[dev]) >> dev) == 1;
}

/**
 * Gestisce l'interrupt dovuto alla scadenza del timeslice
 */
void PLTInterruptHandler() {
    copyRegisters(&current_process->p_s, currentState);
    current_process->p_time += TIMESLICE;
    insertProcQ(&ready_queue, current_process);
    current_process = NULL;
    schedule();
}

/**
 * Gestisce l'interrupt generato dall'Interval Timer
 */
void ITInterruptHandler() {
    LDIT(PSECOND);
    // svuoto la lista dei processi in attesa dello psudoclock riattivandoli
    while(!emptyProcQ(&pseudoclock_blocked_list)) {
        pcb_PTR toUnblock = removeProcQ(&pseudoclock_blocked_list);
        waiting_count--;
        insertProcQ(&ready_queue, toUnblock);
    }
    if(current_process == NULL)
        schedule();
    else
        LDST(currentState);
}

/**
 * Gestisce l'interrupt generato da un terminal device (sia transm che recv)
 * 
 * @param devStatusReg 
 * @param line 
 * @param dev 
 * @return processo da sbloccare
 */
pcb_PTR termDevInterruptHandler(unsigned int *devStatusReg, unsigned int line, unsigned int dev) {
    // Prendo il device register
    termreg_t *devReg = (termreg_t *)getDevReg(line, dev);
    unsigned short int selector;

    // RICORDARSI CHE STATUS PUO' ANCHE SEGNALARE UN ERRORE
    if((devReg->transm_status & 0xFF) == OKCHARTRANS) {
        // Salvo il registro transmitter status
        *devStatusReg = devReg->transm_status;
        devReg->transm_command = ACK;
        selector = 0;
    }
    else if((devReg->recv_status & 0xFF) == OKCHARTRANS) {
        // Salvo il registro receiver status
        *devStatusReg = devReg->recv_status;
        devReg->recv_command = ACK;
        selector = 1;
    }

    // Uso selector per determinare se devo andare a prendere da un transmitter o un receiver
    return removeProcQ(&terminal_blocked_list[selector][dev]);    

}

/**
 * Gestisce l'interrupt generato da un external device
 * 
 * @param devStatusReg 
 * @param line 
 * @param dev 
 * @return processo da sbloccare
 */
pcb_PTR extDevInterruptHandler(unsigned int *devStatusReg, unsigned int line, unsigned int dev) {
    // Prendo il device register
    dtpreg_t *devReg = (dtpreg_t *)getDevReg(line, dev);

    // Salvo status e do l'ACK
    *devStatusReg = devReg->status;
    devReg->command = ACK;

    // Sblocco il pcb che sta aspettando questo ext dev
    return removeProcQ(&external_blocked_list[(line - 3)][dev]);
}