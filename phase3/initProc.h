/*
  Test and Support Level’s global variables
*/

#ifndef INITPROC_H
#define INITPROC_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"

pcb_PTR mutexHolderProcess;
pcb_PTR swapMutexProcess;
state_t swapMutexState; 

// processo di test
pcb_PTR test_pcb;
// indirizzo di memoria corrente
memaddr addr;
// state degli U-proc
state_t uprocStates[UPROCMAX];
// state degli SST
state_t sstStates[UPROCMAX];
// strutture di supporto condivise
support_t supports[UPROCMAX];
// array dei processi SST
pcb_PTR sstArray[UPROCMAX];
// Swap pool 
swpo_t swap_pool[POOLSIZE];

void test();
void initUproc();
void initSST();
void initSwapPool();
void initSwapMutex();
void swapMutex();
void initPageTableEntry(unsigned int asid, pteEntry_t *entry, int idx);

#endif