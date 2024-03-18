#include "interrupt.h"
#include "exceptions.h"

void interruptHandler() {
    
    // Estraggo il cause register
    unsigned int causeReg = currentState->cause;
    unsigned int interruptConsts[] = { LOCALTIMERINT, TIMERINTERRUPT, DISKINTERRUPT, FLASHINTERRUPT, PRINTINTERRUPT, TERMINTERRUPT };
    unsigned int deviceConsts[] = { DEV0ON, DEV1ON, DEV2ON, DEV3ON, DEV4ON, DEV5ON, DEV6ON, DEV7ON };
    
    for(int i = 0; i < 6; i++) {
        if((causeReg & interruptConsts[i]) == 1) {
            switch(i) {
                case 0:
                    setTIMER(TIMESLICE);
                    currentProcess->p_s = *currentState;
                    insertProcQ(&readyQueue->p_list, currentProcess);
                    schedule();
                break;
                case 1:
                    LDIT(PSECOND);
                    pcb_PTR it;
                    while(emptyProcQ(&pseudoclockBlocked->p_list)) {
                        pcb_PTR toUnblock = removeProcQ(&pseudoclockBlocked->p_list);
                        insertProcQ(&readyQueue->p_list, toUnblock);
                    }
                    if(currentProcess == NULL)
                        schedule();
                    else
                        LDST(currentState);
                break;
                default:

                    // Punto alla interrupt line corrente usando la interrupt devices bit map
                    unsigned int *intLaneMapped = (memaddr *)(INTDEVBITMAP + (0x4 * (i - 2)));
                    for(int j = 0; j < 8; j++) {
                        if(((*intLaneMapped) & deviceConsts[j]) == 1) {

                            // Sto trattando un terminal device interrupt   
                            if(i == 5) {
                                termreg_t *devReg = (termreg_t *)getDevReg(i, j);  
                                
                                // Salvo status
                                unsigned int devStatusReg = devReg->recv_status;

                                // Sblocco il pcb che sta aspettando questo ext dev
                            }

                            // Sto trattando un external device interrupt
                            else {
                                
                                dtpreg_t *devReg = (dtpreg_t *)getDevReg(i, j);

                                // Salvo status
                                unsigned int devStatusReg = devReg->status;

                                // Sblocco il pcb che sta aspettando questo term dev                                
                            }
                        }
                    }
                break;
            }
        }
    }

}

memaddr *getDevReg(unsigned int intLine, unsigned int devIntLine) {
    return (memaddr *)(START_DEVREG + (intLine * 0x08) + (devIntLine * 0x10));
}
