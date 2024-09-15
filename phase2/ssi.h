/* 
  System Service Interface
*/

#ifndef SSI_H
#define SSI_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"
#include <umps/arch.h>

void SSIHandler();
static unsigned int createProcess(ssi_create_process_t *arg, pcb_t *sender);
void terminateProcess(pcb_t *p);
void terminateProgeny(pcb_t *p);
void destroyProcess(pcb_t *p);
static void blockForDevice(ssi_do_io_t *arg, pcb_t *toBlock);

#endif