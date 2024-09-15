#include "initProc.h"

extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern void SSTInitialize();
extern void supportExceptionHandler();
extern void pager();

/**
 * Funzione di test per la fase 3
 */
void test() {
  test_pcb = current_process;
  RAMTOP(addr);
  // spostamento oltre i processi ssi e test
  addr -= (3*PAGESIZE);

  // Inizializzo swap pool
  initSwapPool();

  // Inizializzo U-proc
  initUproc();

  // Inizializzo processo mutex
  initSwapMutex();

  // Inizializzo le SST
  initSST();

  // Aspetto gli 8 messaggi che segnalano la terminazione degli U-proc
  for (int i = 0; i < 8; i++) {
    SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
  }

  // Terminazione del processo test
  ssi_payload_t termPayload = {
    .service_code = TERMPROCESS,
    .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &termPayload, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, 0, 0);

  // Se termina correttamente non dovrebbe mai arrivare qui
  PANIC();
}

/**
 * Inizializza gli state di ogni U-proc
 */
static void initUproc() {
  for (int asid = 1; asid <= 8; asid++) {
    uprocStates[asid - 1].pc_epc = (memaddr) UPROCSTARTADDR;
    uprocStates[asid - 1].reg_t9 = (memaddr) UPROCSTARTADDR;
    uprocStates[asid - 1].reg_sp = (memaddr) USERSTACKTOP;
    uprocStates[asid - 1].status = ALLOFF | USERPON | IEPON | IMON | TEBITON;
    uprocStates[asid - 1].entry_hi = asid << ASIDSHIFT;

    // init support
    supports[asid - 1].sup_asid = asid;
    supports[asid - 1].sup_exceptContext[PGFAULTEXCEPT].stackPtr = (memaddr) addr;
    supports[asid - 1].sup_exceptContext[PGFAULTEXCEPT].status = ALLOFF | IEPON | IMON | TEBITON;
    supports[asid - 1].sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr) pager;
    addr -= PAGESIZE;
    supports[asid - 1].sup_exceptContext[GENERALEXCEPT].stackPtr = (memaddr) addr;
    supports[asid - 1].sup_exceptContext[GENERALEXCEPT].status = ALLOFF | IEPON | IMON | TEBITON;
    supports[asid - 1].sup_exceptContext[GENERALEXCEPT].pc = (memaddr) supportExceptionHandler;
    addr -= PAGESIZE;
    for (int i = 0; i < USERPGTBLSIZE; i++) {
      initPageTableEntry(asid, &(supports[asid - 1].sup_privatePgTbl[i]), i);
    }
  }
}

/**
 * Inizializza la swap pool
 */
static void initSwapPool() {
  for (int i = 0; i < POOLSIZE; i++) {
    swap_pool[i].swpo_asid = NOPROC;
    swap_pool[i].swpo_page = -1;
  }
}

/**
 * Inizializza e crea i processi SST
 */
static void initSST() {
  for (int asid = 1; asid <= 8; asid++) {
    // init state
    sstStates[asid - 1].reg_sp = (memaddr) addr;
    sstStates[asid - 1].pc_epc = (memaddr) SSTInitialize;
    sstStates[asid - 1].reg_t9 = (memaddr) SSTInitialize;
    sstStates[asid - 1].status = ALLOFF | IEPON | IMON | TEBITON;
    sstStates[asid - 1].entry_hi = asid << ASIDSHIFT;

    // Creo il processo facendo una richiesta alla SSI
    ssi_create_process_t create = {
      .state = &sstStates[asid - 1],
      .support = &supports[asid - 1],
    };
    ssi_payload_t createPayload = {
      .service_code = CREATEPROCESS,
      .arg = &create,
    };
    SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &createPayload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &sstArray[asid - 1], 0);
    
    addr -= PAGESIZE;
  }
}

/**
 * Inizializza una entry della Page Table
 * 
 * @param asid Identificatore del U-proc
 * @param entry Puntatore alla entry da inizializzare
 * @param idx Indice della entry
 */
static void initPageTableEntry(unsigned int asid, pteEntry_t *entry, int idx){
  if (idx < 31)
    entry->pte_entryHI = KUSEG + (idx << VPNSHIFT) + (asid << ASIDSHIFT);
  else
    entry->pte_entryHI = 0xBFFFF000 + (asid << ASIDSHIFT); 
  entry->pte_entryLO = DIRTYON;
}

/**
 * Inizializza il processo swapMutex
 */
static void initSwapMutex() {
  swapMutexState.reg_sp = (memaddr) addr;
  swapMutexState.pc_epc = swapMutexState.reg_t9 = (memaddr) swapMutex;
  swapMutexState.status = ALLOFF | IEPON | IMON | TEBITON;

  addr -= PAGESIZE;

  // Creo il processo facendo una richiesta alla SSI
  ssi_create_process_t create = {
      .state = &swapMutexState,
      .support = NULL,
  };
  ssi_payload_t createPayload = {
      .service_code = CREATEPROCESS,
      .arg = &create,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &createPayload, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &swapMutexProcess, 0);
  
}

/**
 * Gestisce l'assegnazione della mutua esclusione
 */
void swapMutex() {
  while(TRUE) {
    unsigned int sender = SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
    mutexHolderProcess = (pcb_t *)sender;
    SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
    // Il processo che possiede la mutex si deve occupare di rilasciarla
    SYSCALL(RECEIVEMESSAGE, sender, 0, 0);
    mutexHolderProcess = NULL;
  }
}