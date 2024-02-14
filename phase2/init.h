/*
  Performs Nucleus initialization
*/

#ifndef INIT_H
#define INIT_H

#define MAXTERMINALDEV 8

#include "../headers/const.h"
#include "../headers/types.h"

extern void uTLB_RefillHandler();
extern void exceptionHandler();

// number of started processes not yet terminated
unsigned int processCount;
// number of waiting processes
unsigned int waitingCount;
// running process
pcb_PTR currentProcess;
// queue of PCBs in ready state
pcb_PTR readyQueue;
// list of blocked PCBs for every external device
pcb_PTR externalBlocked[SEMDEVLEN - 1];
// pseudo-clock list
pcb_PTR pseudoclockBlocked;
// list of blocked PCBs for every terminal device
pcb_PTR terminalBlocked[2][MAXTERMINALDEV];

void initialize();

#endif