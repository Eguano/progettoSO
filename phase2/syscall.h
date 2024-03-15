/*
    Syscall exception handler   
*/
#ifndef SYSCALL_H
#define SYSCALL_H

#include "exceptions.h"
#include "../phase1/headers/pcb.h"
#include "init.h"
#include "../phase1/headers/msg.h"
#include "ssi.h"

void syscallHandler();

void sendMessage();
void receiveMessage();

#endif
