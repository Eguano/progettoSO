/*
  System Service Thread
*/

#ifndef SST_H
#define SST_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"

support_t *getSupport();
void SSTInitialize(state_t *s);
void SSTHandler(int asid);
void terminate();
void writePrinter(int asid, sst_print_PTR arg);
void writeTerminal(int asid, sst_print_PTR arg);

#endif