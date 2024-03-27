/*
    Syscall exception handler   
*/
#ifndef SYSCALL_H
#define SYSCALL_H

#include <umps/libumps.h>
#include "../headers/types.h"

void syscallHandler();

void sendMessage();
void receiveMessage();

void passUpOrDie(int);

static msg_PTR createMessage(pcb_PTR sender, unsigned int payload);

#endif
