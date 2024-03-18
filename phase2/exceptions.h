/*
  Exceptions handler (TLB, Program Trap and SYSCALL)
*/

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "../headers/const.h"
#include "../headers/types.h"
#include "syscall.h"

void uTLB_RefillHandler();
void exceptionHandler();

extern state_t *currentState;

#endif