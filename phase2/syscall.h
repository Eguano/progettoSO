/*
    Syscall exception handler   
*/
#ifndef SYSCALL_H
#define SYSCALL_H

#include <umps/libumps.h>
#include "../headers/types.h"

void syscallHandler();
void passUpOrDie(int);
void sendMessage();
void receiveMessage();

#endif
