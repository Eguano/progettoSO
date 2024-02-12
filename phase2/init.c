#include "init.h"

void initialize() {
  // Pass Up Vector for Processor 0
  passupvector_t *passUpVec = (passupvector_t *) PASSUPVECTOR;
  // TODO: decommentare una volta finita la section 3
  // passUpVec->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
  passUpVec->tlb_refill_stackPtr = (memaddr) KERNELSTACK;
  // TODO: aggiungere nome della funzione di gestione eccezioni
  // passUpVec->exception_handler = (memaddr) ...
  passUpVec->exception_stackPtr = (memaddr) KERNELSTACK;

  // level 2 structures
  initPcbs();
  initMsgs();

  // initialize variables
  processCount = 0;
  waitingCount = 0;
  currentProcess = NULL;
  mkEmptyProcQ(&readyQueue->p_list);
  for (int i = 0; i < SEMDEVLEN - 1; i++) {
    mkEmptyProcQ(&externalBlocked[i]->p_list);
  }
  mkEmptyProcQ(&pseudoclockBlocked->p_list);
  // TODO: decommentare dopo numero di terminal device
  /* for (int i = 0; i < ???; i++) {
    mkEmptyProcQ(&terminalBlocked[0][i]->p_list);
    mkEmptyProcQ(&terminalBlocked[1][i]->p_list);
  } */
}