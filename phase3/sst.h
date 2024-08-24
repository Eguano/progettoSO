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
void SSTHandler();

#endif