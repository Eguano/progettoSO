/*
  Exceptions handler (TLB, Program Trap and SYSCALL)
*/

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"

void uTLB_RefillHandler();
void exceptionHandler();

void memcpy(memaddr *, memaddr *, unsigned int);

#endif