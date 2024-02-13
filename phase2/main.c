/*
  uPandOS entry point
*/

#include "init.h"

extern void test();

void main() {
  // nucleus initialization
  initialize();

  // load Interval Timer 100ms
  LDIT(PSECOND);

  // istantiate a first process
  pcb_PTR firstPcb = allocPcb();
  firstPcb->p_s.status = IEPON | IMON;
  RAMTOP(firstPcb->p_s.reg_sp); // DEBUG: potrebbe essere sbagliato (!!!)
  // TODO: aggiungere nome funzione SSI_function_entry_point
  // firstPcb->p_s.pc_epc = (memaddr) ...
  // firstPcb->p_s.reg_t9 = (memaddr) ...
  insertProcQ(&readyQueue->p_list, firstPcb);
  processCount++;

  // istantiate a second process
  pcb_PTR secondPcb = allocPcb();
  secondPcb->p_s.status = IEPON | IMON | TEBITON;
  // TODO: settare lo StackPointer section 1.7
  secondPcb->p_s.pc_epc = (memaddr) test;
  secondPcb->p_s.reg_t9 = (memaddr) test;
  insertProcQ(&readyQueue->p_list, secondPcb);
  processCount++;

  // call the scheduler
  schedule();
}