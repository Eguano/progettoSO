/*
  Performs Nucleus initialization and implements main
*/

#ifndef INIT_H
#define INIT_H

#include "../headers/const.h"
#include "../headers/types.h"

// number of started processes not yet terminated
int process_count;
// number of waiting processes (soft-blocked)
int waiting_count;
// running process
pcb_PTR current_process;
// queue of PCBs in ready state
pcb_PTR ready_queue;
// a list of blocked PCBs for every external device
pcb_PTR external_blocked_list[4][MAXDEV];
// list of blocked PCBs for the pseudo-clock
pcb_PTR pseudoclock_blocked_list;
// list of blocked PCBs for every terminal (transmitter and receiver)
pcb_PTR terminal_blocked_list[2][MAXDEV];
// SSI process
pcb_PTR ssi_pcb;
// p2test process
pcb_PTR p2test_pcb;

static void initialize();

#endif