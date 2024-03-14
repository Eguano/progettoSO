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

                    // Punto alla interrupt lane corrente usando la interrupt devices bit map
                    unsigned int *intLaneMapped = (unsigned int *)(INTDEVBITMAP + (WORDSIZE * (i - 2)));
                    for(int j = 0; j < 8; j++) {
                        if(((*intLaneMapped) & deviceConsts[j]) == 1) {

                            // Punto al device register del device che invoca l'interrupt
                            unsigned int *devReg = (unsigned int *)(START_DEVREG + (i * 0x08) + (j * 0x10));  

                            /** 
                             * In teoria i device register hanno questa forma:
                             * 0000 0000 0000 0000 [DATA1] [DATA0] [COMMAND] [STATUS]
                             * Quindi per prendere solo la parte status maschero tutti i bit
                             * tranne i primi 4
                            */
                            unsigned int devStatusReg = (*devReg) & 0xF;

                            /**
                             * Per settare i bit di COMMAND a ACK, inserisco prima di tutto l'ACK
                             * nella posizione corretta all'interno di 32 bit tutti settati a 0,
                             * poi maschero il contenuto di tutto devReg in modo da avere tutti i
                             * bit all'infuori del segmento COMMAND. Poi mi basta fare l'or bit a bit
                             * per unire devReg con COMMAND mascherato a ACK, andando a sovrascrivere
                             * COMMAND.
                            */
                            *devReg = (*devReg & ~0xF0) | (0x00000000 | (ACK << 4));
                        }
                    }
                break;
            }
        }
    }

}
