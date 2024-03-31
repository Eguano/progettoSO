/* 
  System Service Interface
*/

#ifndef SSI_H
#define SSI_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"

void SSIHandler();
static unsigned int createProcess(ssi_create_process_t *arg, pcb_t *sender);
void terminateProcess(pcb_t *proc);
static void terminateProgeny(pcb_t *p);
static void destroyProcess(pcb_t *p);
static void blockForDevice(ssi_do_io_t *arg, pcb_t *toBlock);
void copyRegisters(state_t *dest, state_t *src);

#endif