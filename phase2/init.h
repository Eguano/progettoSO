/*
  Performs Nucleus initialization
*/

#ifndef INIT_C
#define INIT_C

#include "../headers/const.h"
#include "../headers/types.h"
#include "../phase1/headers/pcb.h"

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
// TODO: pcb_PTR terminalBlocked[2][???];

void initialize();

#endif