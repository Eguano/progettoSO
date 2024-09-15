/*
  System Service Thread
*/

#ifndef SST_H
#define SST_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"
#include <umps/arch.h>

void SSTInitialize();
void SSTHandler(int asid);
void terminate(int asid);
void writePrinter(int asid, sst_print_PTR arg);
void writeTerminal(int asid, sst_print_PTR arg);

#endif