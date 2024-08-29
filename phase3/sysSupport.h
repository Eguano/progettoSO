/*
  Support Level’s:
  - General exception handler,
  - SYSCALL exception handler,
  - Program Trap exception handler.
*/

#ifndef SYSSUPPORT_H
#define SYSSUPPORT_H

#include <umps/libumps.h>
#include "../headers/types.h"
#include "../headers/const.h"

void supportExceptionHandler();
void supportSyscallHandler(state_t *supExceptionState);
void sendMsg(state_t *supExceptionState);
void receiveMsg(state_t *supExceptionState);
void supportTrapHandler(state_t *supExceptionState);

#endif