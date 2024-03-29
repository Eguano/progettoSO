/*
  Performs Nucleus initialization and implements main
*/

#ifndef INIT_H
#define INIT_H

#include "../headers/const.h"
#include "../headers/types.h"

// number of started processes not yet terminated
int process_count;
// number of soft-blocked processes (waiting for DOIO or PseudoClock)
int waiting_count;
// running process
pcb_PTR current_process;
// queue of PCBs in ready state
struct list_head ready_queue;
// TODO: sostituire le liste dei device con semplici puntatori (non fondamentale)
// a list of blocked PCBs for every external device
struct list_head external_blocked_list[4][MAXDEV];
// list of blocked PCBs for the pseudo-clock
struct list_head pseudoclock_blocked_list;
// list of blocked PCBs for every terminal (transmitter and receiver)
struct list_head terminal_blocked_list[2][MAXDEV];
// SSI process
pcb_PTR ssi_pcb;
// p2test process
pcb_PTR p2test_pcb;
// processor0's state at exception time
state_t *currentState;

// DEBUG: traced region
int debug;

static void initialize();
int isInDevicesLists(pcb_t *p);

#endif