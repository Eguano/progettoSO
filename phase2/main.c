/*
  uPandOS entry point
*/

#include "init.h"

void main() {
  // nucleus initialization
  initialize();

  // load Interval Timer 100ms
  LDIT(PSECOND);

  // alloc a first process
  pcb_PTR firstPcb = allocPcb();
  firstPcb->p_s.status = IEPON | IMON | 
  // TODO: Ivan proseguire da qui
}