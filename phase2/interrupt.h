/*
  Interrupt handler
*/

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"

extern void schedule();

extern pcb_PTR current_process;
extern int waiting_count;
extern struct list_head ready_queue;
extern struct list_head pseudoclock_blocked_list;
extern struct list_head external_blocked_list[4][MAXDEV];
extern struct list_head terminal_blocked_list[2][MAXDEV];
extern pcb_PTR ssi_pcb;
extern state_t *currentState;

unsigned int interruptConsts[] = { LOCALTIMERINT, TIMERINTERRUPT, DISKINTERRUPT, FLASHINTERRUPT, PRINTINTERRUPT, TERMINTERRUPT };
unsigned int deviceConsts[] = { DEV0ON, DEV1ON, DEV2ON, DEV3ON, DEV4ON, DEV5ON, DEV6ON, DEV7ON };

void interruptHandler();
memaddr *getDevReg(unsigned int intLine, unsigned int devIntLine);
unsigned short int intLineActive(unsigned short int line);
unsigned short int intPendingInLine(unsigned int causeReg, unsigned short int line);
unsigned short int intPendingOnDev(unsigned int *intLaneMapped, unsigned int dev);
void PLTInterruptHandler();
void ITInterruptHandler();
pcb_PTR termDevInterruptHandler(unsigned int *devStatusReg, unsigned int line, unsigned int dev);
pcb_PTR extDevInterruptHandler(unsigned int *devStatusReg, unsigned int line, unsigned int dev);

#endif