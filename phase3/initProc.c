#include "initProc.h"

extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern void SSTInitialize();

/**
 * Funzione di test per la fase 3
 */
void test() {
  test_pcb = current_process;
  RAMTOP(addr);
  // spostamento oltre i processi ssi e test
  addr -= 3*PAGESIZE;

  /* TODO: Initialize the Level 4/Phase 3 data structures.
  (The Swap Pool table and a Swap Mutex process) */
  
  initUprocState();

  initSST();

  // aspetta 8 messaggi che segnalano la terminazione degli U-proc
  for (int i = 0; i < UPROCMAX; i++) {
    SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
  }
  // terminazione del processo test
  ssi_payload_t termPayload = {
    .service_code = TERMPROCESS,
    .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &termPayload, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, 0, 0);

  // se termina correttamente non dovrebbe mai arrivare qui
  PANIC();
}

/**
 * Inizializza gli state di ogni U-proc
 */
void initUprocState() {
  for (int asid = 1; asid <= UPROCMAX; asid++) {
    uprocStates[asid - 1].pc_epc = uprocStates[asid - 1].reg_t9 = (memaddr) UPROCSTARTADDR;
    uprocStates[asid - 1].reg_sp = (memaddr) USERSTACKTOP;
    uprocStates[asid - 1].status = ALLOFF | USERPON | IEPON | IMON | TEBITON;
    uprocStates[asid - 1].entry_hi = asid << ASIDSHIFT; 
  }
}

/**
 * Inizializza e crea i processi SST con support
 */
void initSST() {
  for (int asid = 1; asid <= UPROCMAX; asid++) {
    // init state
    sstStates[asid - 1].pc_epc = sstStates[asid - 1].reg_t9 = (memaddr) SSTInitialize;
    sstStates[asid - 1].reg_sp = (memaddr) addr;
    sstStates[asid - 1].status = ALLOFF | IEPON | IMON | TEBITON;
    sstStates[asid - 1].entry_hi = asid << ASIDSHIFT;
    addr -= PAGESIZE;
    // init support
    supports[asid - 1].sup_asid = asid;
    supports[asid - 1].sup_exceptContext[PGFAULTEXCEPT].stackPtr = (memaddr) addr;
    supports[asid - 1].sup_exceptContext[PGFAULTEXCEPT].status = ALLOFF | IEPON | IMON | TEBITON;
    // TODO: aggiungere indirizzo di funzione pager
    // supports[asid - 1].sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr) pager;
    addr -= PAGESIZE;
    supports[asid - 1].sup_exceptContext[GENERALEXCEPT].stackPtr = (memaddr) addr;
    supports[asid - 1].sup_exceptContext[GENERALEXCEPT].status = ALLOFF | IEPON | IMON | TEBITON;
    // TODO: aggiungere indirizzo funzione di gestione eccezioni
    // supports[asid - 1].sup_exceptContext[GENERALEXCEPT].pc = (memaddr) generalExceptionHandler;
    addr -= PAGESIZE;
    for (int i = 0; i < USERPGTBLSIZE; i++) {
      // TODO: inizializzare una singola entry della tabella delle pagine
    }

    // create sst process
    pcb_PTR p;
    ssi_create_process_t create = {
      .state = &sstStates[asid - 1],
      .support = &supports[asid - 1],
    };
    ssi_payload_t createPayload = {
      .service_code = CREATEPROCESS,
      .arg = &create,
    };
    SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &createPayload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &p, 0);
  }
}