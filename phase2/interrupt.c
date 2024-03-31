#include "interrupt.h"

#include "../phase1/headers/pcb.h"
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
extern void ssiDM(pcb_PTR toUnblock);

extern unsigned int debug;

void interruptHandler() {
    debug = 0x300;
    // Estraggo il cause register
    unsigned int causeReg = currentState->cause;
    debug = currentState->cause;

    // Gli interrupt sono abilitati a livello globale
    if(((currentState->status & IEPON) >> 2) == 1) {
        debug = 0x301;

        for(int line = 0; line < 8; line++) {
            // Se la line e' attiva
            if(intPendingInLine(causeReg, line)) {
                debug = 0x302;
                switch(line) {
                    case 0:
                    break;
                    case 1:
                        debug = 0x303;
                        if(intLineActive(1)) {
                            debug = 0x304;
                            PLTInterruptHandler();
                        }
                    break;
                    case 2:
                        debug = 0x305;
                        if(intLineActive(2)) {
                            debug = 0x306;
                            ITInterruptHandler();
                        }
                    break;
                    case 5:
                    break;
                    default:
                        debug = 0x307;
                        // Controllo se la interrupt line corrente e' attiva facendo & e shiftando a sx di 8 + line
                        if(intLineActive(line)) {
                            debug = 0x308;

                            // Punto alla interrupt line corrente usando la interrupt devices bit map
                            unsigned int *intLaneMapped = (memaddr *)(INTDEVBITMAP + (0x4 * (line - 3)));

                            for(int dev = 0; dev < 8; dev++) {
                                if(intPendingOnDev(intLaneMapped, dev)) {
                                    debug = 0x309;
                                    
                                    pcb_PTR toUnblock;
                                    unsigned int devStatusReg;

                                    // Sto trattando un terminal device interrupt   
                                    if(line == 7) {
                                        debug = 0x310;
                                        toUnblock = termDevInterruptHandler(&devStatusReg, line, dev);
                                    }
                                    // Sto trattando un external device interrupt
                                    else {
                                        debug = 0x311;
                                        toUnblock = extDevInterruptHandler(&devStatusReg, line, dev);
                                    }
                                    
                                    if(toUnblock == NULL) {
                                        debug = 0x312;
                                        if(current_process == NULL)
                                            schedule();
                                        else
                                            LDST(currentState);
                                    }
                                    else {
                                        debug = 0x313;
                                        // decremento processi in attesa
                                        waiting_count--;
                                        // Salvo lo status register su v0 del processo da sbloccare
                                        toUnblock->p_s.reg_v0 = devStatusReg;
                                        // messaggio all'ssi per far sbloccare il processo
                                        ssiDM(toUnblock);

                                        if(current_process == NULL)
                                            schedule();
                                        else
                                            LDST(currentState);
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
    return (memaddr *)(START_DEVREG + ((intLine - 3)* 0x80) + (devIntLine * 0x10));
}

unsigned short int intLineActive(unsigned short int line) {
    return (((currentState->status & IMON) >> (8 + line)) & 0x1) == 1; 
}

unsigned short int intPendingInLine(unsigned int causeReg, unsigned short int line) {
    return (((causeReg & interruptConsts[line]) >> (8 + line)) & 0x1) == 1;
}

unsigned short int intPendingOnDev(unsigned int *intLaneMapped, unsigned int dev) {
    return (((*intLaneMapped) & deviceConsts[dev]) >> dev) == 1;
}

void PLTInterruptHandler() {
    debug = 0x314;
    copyRegisters(&current_process->p_s, currentState);
    current_process->p_time += TIMESLICE;
    insertProcQ(&ready_queue, current_process);
    current_process = NULL;
    schedule();
}

void ITInterruptHandler() {
    debug = 0x315;
    LDIT(PSECOND);
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

pcb_PTR termDevInterruptHandler(unsigned int *devStatusReg, unsigned int line, unsigned int dev) {
    debug = 0x316;
    // Prendo il device register
    termreg_t *devReg = (termreg_t *)getDevReg(line, dev);
    unsigned short int selector;

    // RICORDARSI CHE STATUS PUO' ANCHE SEGNALARE UN ERRORE
    if((devReg->transm_status & 0xFF) == OKCHARTRANS) {
        debug = 0x317;
        // Salvo il registro transmitter status
        *devStatusReg = devReg->transm_status;
        devReg->transm_command = ACK;
        selector = 0;
    }
    else if((devReg->recv_status & 0xFF) == OKCHARTRANS) {
        debug = 0x318;
        // Salvo il registro reciever status
        *devStatusReg = devReg->recv_status;
        devReg->recv_command = ACK;
        selector = 1;
    }

    // Uso selector per determinare se devo andare a prendere da un transmitter o un reciever
    return removeProcQ(&terminal_blocked_list[selector][dev]);    

}

pcb_PTR extDevInterruptHandler(unsigned int *devStatusReg, unsigned int line, unsigned int dev) {
    debug = 0x319;
    // Prendo il device register
    dtpreg_t *devReg = (dtpreg_t *)getDevReg(line, dev);

    // Salvo status e do l'ACK
    *devStatusReg = devReg->status;
    devReg->command = ACK;

    // Sblocco il pcb che sta aspettando questo ext dev
    return removeProcQ(&external_blocked_list[(line - 3)][dev]);
}


void interruptDEBUG() {
    debug = 0x320;
    int line = 7;
    // Punto alla interrupt line corrente usando la interrupt devices bit map
    unsigned int *intLaneMapped = (memaddr *)(INTDEVBITMAP + (0x4 * (line - 3)));

    for(int dev = 0; dev < 8; dev++) {
        if(intPendingOnDev(intLaneMapped, dev)) {
            debug = 0x321;
            
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
            
            if(toUnblock == NULL) {
                debug = 0x322;
                if(current_process == NULL)
                    schedule();
                else
                    LDST(currentState);
            }
            else {
                debug = 0x323;
                // decremento processi in attesa
                waiting_count--;
                // Salvo lo status register su v0 del processo da sbloccare
                toUnblock->p_s.reg_v0 = devStatusReg;
                // messaggio all'ssi per far sbloccare il processo
                ssiDM(toUnblock);

                if(current_process == NULL)
                    schedule();
                else
                    LDST(currentState);
            }
        }
    }
}