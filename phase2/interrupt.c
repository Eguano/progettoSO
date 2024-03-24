#include "interrupt.h"

#include "../phase1/headers/pcb.h"

void interruptHandler() {
    
    // Estraggo il cause register
    unsigned int causeReg = currentState->cause;

    // Gli interrupt sono abilitati a livello globale
    if((currentState->status & IECON) == 1) {

        for(int line = 0; line < 6; line++) {
            // Se la line e' attiva
            if(intPendingInLine(causeReg, line)) {
                switch(line) {
                    case 0:
                        if(intLineActive(1)) {
                            PLTInterruptHandler();
                        }
                    break;
                    case 1:
                        if(intLineActive(2)) {
                            ITInterruptHandler();
                        }
                    break;
                    default:
                        // Controllo se la interrupt line corrente e' attiva facendo & e shiftando a sx di 8 + line + 1
                        if(intLineActive(line + 1)) {
                            // Punto alla interrupt line corrente usando la interrupt devices bit map
                            unsigned int *intLaneMapped = (memaddr *)(INTDEVBITMAP + (0x4 * (line - 2)));

                            for(int dev = 0; dev < 8; dev++) {
                                if(intPendingOnDev(intLaneMapped, dev)) {
                                    
                                    pcb_PTR toUnblock;
                                    unsigned int devStatusReg;

                                    // Sto trattando un terminal device interrupt   
                                    if(line == 5) {
                                        toUnblock = termDevInterruptHandler(&devStatusReg, line, dev);
                                    }

                                    // Sto trattando un external device interrupt
                                    else {
                                        toUnblock = extDevInterruptHandler(&devStatusReg, line, dev);
                                    }
                                    
                                    if(toUnblock == NULL) {
                                        LDST(currentState); // !!! da sostituire con schedule?
                                    }
                                    else {
                                        /* // Backup del currentState, faccio la SYSCALL e ripristino il currentState
                                        state_t curStateBackup = *((state_t *)BIOSDATAPAGE);
                                        SYSCALL(SENDMESSAGE, (unsigned int)toUnblock, (unsigned int)ACK, 0);
                                        *((state_t *)BIOSDATAPAGE) = curStateBackup;    // scary

                                        // Salvo lo status register su v0 del processo da sbloccare
                                        toUnblock->p_s.reg_v0 = devStatusReg;

                                        waiting_count--;
                                        insertProcQ(&ready_queue, toUnblock);

                                        LDST(currentState); // !!! da sostituire con schedule? */


                                        // MODIFICHE IVAN 23-03
                                        // decremento processi in attesa
                                        waiting_count--;
                                        // Backup del currentState, faccio la SYSCALL e ripristino il currentState
                                        state_t curStateBackup = *((state_t *)BIOSDATAPAGE);

                                        // Salvo lo status register su v0 del processo da sbloccare
                                        toUnblock->p_s.reg_v0 = devStatusReg;

                                        ssi_end_io_t endio = {
                                            .status = devStatusReg,
                                            .toUnblock = toUnblock,
                                        };
                                        ssi_payload_t payload = {
                                            .service_code = ENDIO,
                                            .arg = &endio,
                                        };

                                        // TODO: c'è un modo per mandare messaggi senza usare SYSCALL()?
                                        SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&payload, 0);

                                        *((state_t *)BIOSDATAPAGE) = curStateBackup;    // scary

                                        LDST(currentState); // !!! da sostituire con schedule?

                                    }
                                }
                            }
                        }
                    break;
                }
            }
        }
    }
}

memaddr *getDevReg(unsigned int intLine, unsigned int devIntLine) {
    return (memaddr *)(START_DEVREG + (intLine * 0x08) + (devIntLine * 0x10));
}

unsigned short int intLineActive(unsigned short int line) {
    return (((currentState->status & IMON) >> (8 + line)) & 0x1) == 1;
}

unsigned short int intPendingInLine(unsigned int causeReg, unsigned short int line) {
    return (((causeReg & interruptConsts[line]) >> (8 + line)) & 0x1) == 1;
}

unsigned short int intPendingOnDev(unsigned int *intLaneMapped, unsigned int dev) {
    return ((*intLaneMapped) & deviceConsts[dev]) == 1;
}

void PLTInterruptHandler() {
    setTIMER(TIMESLICE);
    current_process->p_s = *currentState;
    insertProcQ(&ready_queue, current_process);
    current_process = NULL;
    schedule();
}

void ITInterruptHandler() {
    LDIT(PSECOND);
    // pcb_PTR it; commentato da ivan
    while(emptyProcQ(&pseudoclock_blocked_list)) {
        pcb_PTR toUnblock = removeProcQ(&pseudoclock_blocked_list);
        waiting_count--;
        insertProcQ(&ready_queue, toUnblock);
    }
    if(current_process == NULL)
        schedule();
    else
        LDST(currentState);
}

pcb_PTR termDevInterruptHandler(unsigned int *devStatusReg, unsigned int line, unsigned int dev) {
    // Prendo il device register
    termreg_t *devReg = (termreg_t *)getDevReg(line, dev);
    unsigned short int selector;

    // RICORDARSI CHE STATUS PUO' ANCHE SEGNALARE UN ERRORE
    if(devReg->transm_status == OKCHARTRANS) {
        // Salvo il registro transmitter status
        *devStatusReg = devReg->transm_status;
        devReg->transm_command = ACK;
        selector = 0;
    }
    else if(devReg->recv_status == OKCHARTRANS) {
        // Salvo il registro reciever status
        *devStatusReg = devReg->recv_status;
        devReg->recv_command = ACK;
        selector = 1;
    }

    // Uso selector per determinare se devo andare a prendere da un transmitter o un reciever
    return removeProcQ(&terminal_blocked_list[selector][dev]);    

}

pcb_PTR extDevInterruptHandler(unsigned int *devStatusReg, unsigned int line, unsigned int dev) {
    // Prendo il device register
    dtpreg_t *devReg = (dtpreg_t *)getDevReg(line, dev);

    // Salvo status e do l'ACK
    *devStatusReg = devReg->status;
    devReg->command = ACK;

    // Mando un messaggio e sblocco il pcb che sta aspettando questo ext dev
    return removeProcQ(&external_blocked_list[line-1][dev]);
}
