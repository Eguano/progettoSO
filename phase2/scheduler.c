#include "scheduler.h"

/**
 * Carica un processo per essere mandato in run, altrimenti blocca l'esecuzione.
 * <p>Salvare lo stato, rimuovere il processo che ha terminato ecc
 * viene svolto dall'interrupt handler
 */
void schedule() {
  if (emptyProcQ(readyQueue)) {
    if (processCount == 1) {
      // only SSI in the system
      HALT();
    } else if (processCount > 0 && waitingCount > 0) {
      // waiting for an interrupt
      setSTATUS((getSTATUS() | IECON | IMON) & !TEBITON);
      WAIT();
    } else if (processCount > 0 && waitingCount == 0) {
      // deadlock
      PANIC();
    }
  }
  /* nuovo processo dopo i controlli in modo che in caso di WAIT
  nel momento in cui si riparte si possa caricare il processo */
  // dispatch the next process
  currentProcess = removeProcQ(&readyQueue->p_list);
  // load the PLT
  setTIMER(TIMESLICE);
  // perform Load Processor State
  LDST(&currentProcess->p_s);
}