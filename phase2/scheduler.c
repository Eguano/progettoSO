#include "scheduler.h"

#include "../phase1/headers/pcb.h"

extern int process_count;
extern int waiting_count;
extern pcb_PTR current_process;
extern struct list_head ready_queue;
extern void interruptHandler();

extern unsigned int debug;
extern void interruptDEBUG();

/**
 * Carica un processo per essere mandato in run, altrimenti blocca l'esecuzione.
 * <p>Salvare lo stato, rimuovere il processo che ha terminato ecc viene svolto dal chiamante
 */
void schedule() {
  debug = 0x400;
  if (emptyProcQ(&ready_queue)) {
    if (process_count == 1) {
      // only SSI in the system
      HALT();
    } else if (process_count > 0 && waiting_count > 0) {
      // waiting for an interrupt
      debug = 0x401;
      setSTATUS((getSTATUS() | IECON | IMON) & !TEBITON);
      WAIT();
      // TODO: da generalizzare
      interruptDEBUG();
    } else if (process_count > 0 && waiting_count == 0) {
      // deadlock
      PANIC();
    }
  }
  /* nuovo processo dopo i controlli in modo che in caso di WAIT
  nel momento in cui si riparte si possa caricare il processo */
  // dispatch the next process
  current_process = removeProcQ(&ready_queue);
  // load the PLT
  setTIMER(TIMESLICE);
  debug = 0x402;
  // perform Load Processor State
  LDST(&current_process->p_s);
}