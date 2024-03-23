/* 
  System Service Interface
*/

#ifndef SSI_H
#define SSI_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"

void SSIRequest(pcb_t* sender, int service, void* arg);
void SSIHandler();
static pcb_PTR createProcess(ssi_create_process_t *arg, pcb_t *sender);
static void terminateProcess(pcb_t *proc);
static void terminateProgeny(pcb_t *p);
static void destroyProcess(pcb_t *p);
static int findDevice(memaddr *commandAddr);
static int findInstance(memaddr distFromBase);

#endif