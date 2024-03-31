/*
  Interrupt handler
*/

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"

unsigned int interruptConsts[] = { INTPROCINTERRUPT, LOCALTIMERINT, TIMERINTERRUPT, DISKINTERRUPT, FLASHINTERRUPT, NETWORKINTERRUPT, PRINTINTERRUPT, TERMINTERRUPT };
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