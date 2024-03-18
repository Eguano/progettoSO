/*
  Interrupt handler
*/

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"

void interruptHandler();
memaddr *getDevReg(unsigned int intLine, unsigned int devIntLine);

#endif