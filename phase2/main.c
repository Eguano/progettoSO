/*
  uPandOS entry point
*/

#include "init.h"

extern void SSIHandler();
extern void test();

/**
 * Entry point del sistema operativo.
 * <p>Inizializza il nucleo, istanzia il processo SSI e test
 * e carica l'Interval Timer
 */
void main() {
  // nucleus initialization
  initialize();

  // load Interval Timer 100ms
  LDIT(PSECOND);

  // istantiate a first process
  pcb_PTR firstPcb = allocPcb();
  firstPcb->p_s.status = IEPON | IMON;
  RAMTOP(firstPcb->p_s.reg_sp);
  firstPcb->p_s.pc_epc = firstPcb->p_s.reg_t9 = (memaddr) SSIHandler;
  insertProcQ(&readyQueue->p_list, firstPcb);
  processCount++;
  ssiAddress = &firstPcb;

  // istantiate a second process
  pcb_PTR secondPcb = allocPcb();
  secondPcb->p_s.status = IEPON | IMON | TEBITON;
  RAMTOP(secondPcb->p_s.reg_sp);
  secondPcb->p_s.reg_sp -= (2 * PAGESIZE);
  secondPcb->p_s.pc_epc = secondPcb->p_s.reg_t9 = (memaddr) test;
  insertProcQ(&readyQueue->p_list, secondPcb);
  processCount++;

  // call the scheduler
  schedule();
}