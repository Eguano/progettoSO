/* 
  System Service Interface
*/

#ifndef SSI_H
#define SSI_H

#include "../headers/const.h"
#include "../headers/types.h"

void SSIRequest(pcb_t* sender, int service, void* arg);

void SSIHandler();

#endif