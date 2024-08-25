/*
  Test and Support Level’s global variables
*/

#ifndef INITPROC_H
#define INITPROC_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"

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

void test();
void initUprocState();
void initSST();

#endif