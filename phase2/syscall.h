/*
    Syscall exception handler   
*/
#ifndef SYSCALL_H
#define SYSCALL_H

#include "exceptions.h"
#include "pcb.h"
#include "init.h"
#include "msg.h"
#include "ssi.h"

void syscallHandler();
void passUpOrDie(int);
void sendMessage();
void receiveMessage();

#endif