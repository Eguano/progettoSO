/*
  Exceptions handler (TLB, Program Trap and SYSCALL)
*/

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

void uTLB_RefillHandler();

void exceptionHandler();

#endif