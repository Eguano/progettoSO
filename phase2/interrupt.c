#include "interrupt.h"

#include "../phase1/headers/pcb.h"

extern int debug;

void interruptHandler() {
    debug = 300;
    
    // Estraggo il cause register
    unsigned int causeReg = currentState->cause;
    debug = 301;

    // Gli interrupt sono abilitati a livello globale
    if((currentState->status & IECON) == 1) {
        debug = 302;

        for(int line = 0; line < 6; line++) {
            debug = 303;
            // Se la line e' attiva
            if(intPendingInLine(causeReg, line)) {
                debug = 304;
                switch(line) {
                    case 0:
                        debug = 305;
                        if(intLineActive(1)) {
                            debug = 306;
                            PLTInterruptHandler();
                            debug = 307;
                        }
                    break;
                    case 1:
                        debug = 308;
                        if(intLineActive(2)) {
                            debug = 309;
                            ITInterruptHandler();
                            debug = 310;
                        }
                    break;
                    default:
                        debug = 311;
                        // Controllo se la interrupt line corrente e' attiva facendo & e shiftando a sx di 8 + line + 1
                        if(intLineActive(line + 1)) {
                            debug = 312;
                            // Punto alla interrupt line corrente usando la interrupt devices bit map
                            unsigned int *intLaneMapped = (memaddr *)(INTDEVBITMAP + (0x4 * (line - 2)));
                            debug = 313;

                            for(int dev = 0; dev < 8; dev++) {
                                debug = 314;
                                if(intPendingOnDev(intLaneMapped, dev)) {
                                    debug = 315;
                                    
                                    pcb_PTR toUnblock;
                                    unsigned int devStatusReg;

                                    // Sto trattando un terminal device interrupt   
                                    if(line == 5) {
                                        debug = 316;
                                        toUnblock = termDevInterruptHandler(&devStatusReg, line, dev);
                                    }

                                    // Sto trattando un external device interrupt
                                    else {
                                        debug = 317;
                                        toUnblock = extDevInterruptHandler(&devStatusReg, line, dev);
                                    }
                                    debug = 318;
                                    
                                    if(toUnblock == NULL) {
                                        debug = 319;
                                        LDST(currentState); // !!! da sostituire con schedule?
                                    }
                                    else {
                                        debug = 320;
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
                                        debug = 321;

                                        // Salvo lo status register su v0 del processo da sbloccare
                                        toUnblock->p_s.reg_v0 = devStatusReg;
                                        debug = 322;

                                        ssi_end_io_t endio = {
                                            .status = devStatusReg,
                                            .toUnblock = toUnblock,
                                        };
                                        ssi_payload_t payload = {
                                            .service_code = ENDIO,
                                            .arg = &endio,
                                        };
                                        debug = 323;

                                        // TODO: c'è un modo per mandare messaggi senza usare SYSCALL()?
                                        SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&payload, 0);
                                        debug = 324;

                                        *((state_t *)BIOSDATAPAGE) = curStateBackup;    // scary
                                        debug = 325;

                                        LDST(currentState); // !!! da sostituire con schedule?
                                        debug = 326;

                                    }
                                    debug = 327;
                                }
                                debug = 328;
                            }
                            debug = 329;
                        }
                        debug = 330;
                    break;
                }
                debug = 331;
            }
            debug = 332;
        }
        debug = 333;
    }
    debug = 334;
}

memaddr *getDevReg(unsigned int intLine, unsigned int devIntLine) {
    debug = 335;
    return (memaddr *)(START_DEVREG + (intLine * 0x08) + (devIntLine * 0x10));
}

unsigned short int intLineActive(unsigned short int line) {
    debug = 336;
    return (((currentState->status & IMON) >> (8 + line)) & 0x1) == 1;
}

unsigned short int intPendingInLine(unsigned int causeReg, unsigned short int line) {
    debug = 337;
    return (((causeReg & interruptConsts[line]) >> (8 + line)) & 0x1) == 1;
}

unsigned short int intPendingOnDev(unsigned int *intLaneMapped, unsigned int dev) {
    debug = 338;
    return ((*intLaneMapped) & deviceConsts[dev]) == 1;
}

void PLTInterruptHandler() {
    debug = 339;
    setTIMER(TIMESLICE);
    current_process->p_s = *currentState;
    insertProcQ(&ready_queue, current_process);
    debug = 340;
    current_process = NULL;
    current_process->p_time += (TIMESLICE - getTIMER());
    schedule();
    debug = 341;
}

void ITInterruptHandler() {
    debug = 342;
    LDIT(PSECOND);
    // pcb_PTR it; commentato da ivan
    while(emptyProcQ(&pseudoclock_blocked_list)) {
        pcb_PTR toUnblock = removeProcQ(&pseudoclock_blocked_list);
        waiting_count--;
        insertProcQ(&ready_queue, toUnblock);
    }
    debug = 343;
    if(current_process == NULL)
        schedule();
    else
        LDST(currentState);

    debug = 344;
}

pcb_PTR termDevInterruptHandler(unsigned int *devStatusReg, unsigned int line, unsigned int dev) {
    debug = 345;
    // Prendo il device register
    termreg_t *devReg = (termreg_t *)getDevReg(line, dev);
    unsigned short int selector;

    // RICORDARSI CHE STATUS PUO' ANCHE SEGNALARE UN ERRORE
    if(devReg->transm_status == OKCHARTRANS) {
        debug = 346;
        // Salvo il registro transmitter status
        *devStatusReg = devReg->transm_status;
        devReg->transm_command = ACK;
        selector = 0;
    }
    else if(devReg->recv_status == OKCHARTRANS) {
        debug = 347;
        // Salvo il registro reciever status
        *devStatusReg = devReg->recv_status;
        devReg->recv_command = ACK;
        selector = 1;
    }
    debug = 348;

    // Uso selector per determinare se devo andare a prendere da un transmitter o un reciever
    return removeProcQ(&terminal_blocked_list[selector][dev]);    

}

pcb_PTR extDevInterruptHandler(unsigned int *devStatusReg, unsigned int line, unsigned int dev) {
    debug = 349;
    // Prendo il device register
    dtpreg_t *devReg = (dtpreg_t *)getDevReg(line, dev);

    // Salvo status e do l'ACK
    *devStatusReg = devReg->status;
    devReg->command = ACK;
    debug = 350;
    // Mando un messaggio e sblocco il pcb che sta aspettando questo ext dev
    return removeProcQ(&external_blocked_list[line-1][dev]);
}
