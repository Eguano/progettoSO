#include "init.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"

extern void uTLB_RefillHandler();
extern void exceptionHandler();
extern void schedule();
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
  debug = 200;

  // load Interval Timer 100ms
  LDIT(PSECOND);
  debug = 201;

  // istantiate a first process
  ssi_pcb = allocPcb();
  ssi_pcb->p_s.status |= IEPON | IMON;
  RAMTOP(ssi_pcb->p_s.reg_sp);
  ssi_pcb->p_s.pc_epc = ssi_pcb->p_s.reg_t9 = (memaddr) SSIHandler;
  insertProcQ(&ready_queue, ssi_pcb);
  process_count++;
  debug = 202;

  // istantiate a second process
  p2test_pcb = allocPcb();
  p2test_pcb->p_s.status |= IEPON | IMON | TEBITON;
  // TODO: possibile errore nel settaggio di RAMTOP
  RAMTOP(p2test_pcb->p_s.reg_sp);
  p2test_pcb->p_s.reg_sp -= (2 * PAGESIZE);
  p2test_pcb->p_s.pc_epc = p2test_pcb->p_s.reg_t9 = (memaddr) test;
  insertProcQ(&ready_queue, p2test_pcb);
  process_count++;
  debug = 203;

  // call the scheduler
  schedule();
}

/**
 * Inizializza il PassUp Vector, le strutture di pcb/msg e
 * le variabili globali
 */
static void initialize() {
  debug = 204;
  // Pass Up Vector for Processor 0
  passupvector_t *passUpVec = (passupvector_t *) PASSUPVECTOR;
  passUpVec->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
  passUpVec->tlb_refill_stackPtr = (memaddr) KERNELSTACK;
  passUpVec->exception_handler = (memaddr) exceptionHandler;
  passUpVec->exception_stackPtr = (memaddr) KERNELSTACK;
  debug = 205;

  // level 2 structures
  initPcbs();
  initMsgs();
  debug = 206;

  // initialize variables
  process_count = 0;
  waiting_count = 0;
  current_process = NULL;
  mkEmptyProcQ(&ready_queue);
  for (int i = 0; i < MAXDEV; i++) {
    // external
    mkEmptyProcQ(&external_blocked_list[0][i]);
    mkEmptyProcQ(&external_blocked_list[1][i]);
    mkEmptyProcQ(&external_blocked_list[2][i]);
    mkEmptyProcQ(&external_blocked_list[3][i]);
    // terminal
    mkEmptyProcQ(&terminal_blocked_list[0][i]);
    mkEmptyProcQ(&terminal_blocked_list[1][i]);
  }
  mkEmptyProcQ(&pseudoclock_blocked_list);
  currentState = (state_t *)BIOSDATAPAGE;
  debug = 206;
}