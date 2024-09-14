/*
  TLB exception handler (The Pager)
*/

#ifndef VMSUPPORT_H
#define VMSUPPORT_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"
#include <umps/arch.h>

void Pager();

static unsigned int selectFrame();
void invalidateFrame(unsigned int frame, support_t *support_PTR);
void updateTLB(pteEntry_t *entry);

int readWriteBackingStore(dtpreg_t *flashDevReg, memaddr dataMemAddr, unsigned int devBlockNo, unsigned int opType);

#endif