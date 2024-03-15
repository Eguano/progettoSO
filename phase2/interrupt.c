#include "interrupt.h"

void interruptHandler() {
    
    state_t *currentState = (state_t *)BIOSDATAPAGE;    
    unsigned int causeReg = getCAUSE();
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
                    for(int j = 0; j < 8; j++) {
                        
                    }
                break;
            }
        }
    }

}
