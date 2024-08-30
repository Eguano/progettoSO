/*
  TLB exception handler (The Pager)
*/

#ifndef VMSUPPORT_H
#define VMSUPPORT_H

#include <umps/libumps.h>
#include "../headers/const.h"
#include "../headers/types.h"
#include "./initProc.h"
#include "./sysSupport.h"

void TLB_ExceptionHandler();

#endif