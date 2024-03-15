/*
  Interrupt handler
*/

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <umps/libumps.h>
#include "init.h"
#include "scheduler.h"
#include "../headers/const.h"
#include "../headers/types.h"
#include "../phase1/headers/pcb.h"

void interruptHandler();

#endif