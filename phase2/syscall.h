/*
    Syscall exception handler   
*/

#ifndef SYSCALL_H
#define SYSCALL_H

#include <umps/libumps.h>
#include "../headers/types.h"
#include "../headers/const.h"

void syscallHandler();
void sendMessage();
void receiveMessage();
void passUpOrDie(int);
msg_PTR createMessage(pcb_PTR sender, unsigned int payload);
int ssiDM(pcb_PTR toUnblock);

#endif