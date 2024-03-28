#include "scheduler.h"

#include "../phase1/headers/pcb.h"

extern int process_count;
extern int waiting_count;
extern pcb_PTR current_process;
extern struct list_head ready_queue;

extern int debug;

/**
 * Carica un processo per essere mandato in run, altrimenti blocca l'esecuzione.
 * <p>Salvare lo stato, rimuovere il processo che ha terminato ecc viene svolto dal chiamante
 */
void schedule() {
  debug = 400;
  if (emptyProcQ(&ready_queue)) {
    debug = 401;
    if (process_count == 1) {
      debug = 402;
      // only SSI in the system
      HALT();
    } else if (process_count > 0 && waiting_count > 0) {
      debug = 403;
      // waiting for an interrupt
      setSTATUS((getSTATUS() | IECON | IMON) & !TEBITON);
      WAIT();
    } else if (process_count > 0 && waiting_count == 0) {
      debug = 404;
      // deadlock
      PANIC();
    }
  }
  debug = 405;
  /* nuovo processo dopo i controlli in modo che in caso di WAIT
  nel momento in cui si riparte si possa caricare il processo */
  // dispatch the next process
  current_process = removeProcQ(&ready_queue);
  // load the PLT
  setTIMER(TIMESLICE);
  debug = 406;
  // perform Load Processor State
  LDST(&current_process->p_s);
  debug = 407;
}