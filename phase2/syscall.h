/*
    Syscall exception handler   
*/
#ifndef SYSCALL_H
#define SYSCALL_H

#include "exceptions.h"

void syscallHandler();
void passUpOrDie(int indexValue);

#endif