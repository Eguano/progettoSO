/*
  Performs Nucleus initialization
*/

#ifndef INIT_H
#define INIT_H
#define MAXTERMINALDEV 8

#include "../headers/const.h"
#include "../headers/types.h"
#include "../phase1/headers/pcb.h"

extern void uTLB_RefillHandler();

// number of started processes not yet terminated
unsigned int processCount;
// number of waiting processes
unsigned int waitingCount;
// running process
pcb_PTR currentProcess;
// queue of PCBs in ready state
pcb_PTR readyQueue;
// list of blocked PCBs for external device
pcb_PTR externalBlocked[SEMDEVLEN - 1];
// pseudo-clock list
pcb_PTR pseudoclockBlocked;
// list of blocked PCBs for terminal device
pcb_PTR terminalBlocked[2][MAXTERMINALDEV];

void initialize();

#endif