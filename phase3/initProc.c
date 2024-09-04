#include "initProc.h"

extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern void SSTInitialize();
extern void supportExceptionHandler();
extern void TLB_ExceptionHandler();

extern unsigned int debug;

/**
 * Funzione di test per la fase 3
 */
void test() {
  test_pcb = current_process;
  RAMTOP(addr);
  // spostamento oltre i processi ssi e test
  addr -= 3*PAGESIZE;

  // swap pool
  initSwapPool();

  initUprocState();

  initSwapMutex();

  initSST();

  // aspetta 8 messaggi che segnalano la terminazione degli U-proc
  for (int i = 0; i < 1; i++) {
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
 * Inizializza la swap pool.
 */
void initSwapPool() {
  // DEBUG:
  // swap_pool[POOLSIZE] = (swpo_t *) FRAMEPOOLSTART;
  for (int i = 0; i < POOLSIZE; i++) {
    swap_pool[i].swpo_asid = NOPROC;
    swap_pool[i].swpo_page = -1;
    // swap_pool[i].swpo_pte_ptr = NULL;
  }
}

/**
 * Inizializza e crea i processi SST con support
 */
void initSST() {
  // DEBUG: un solo processo inizialmente
  for (int asid = 1; asid <= 1; asid++) {
    // init state
    sstStates[asid - 1].pc_epc = sstStates[asid - 1].reg_t9 = (memaddr) SSTInitialize;
    sstStates[asid - 1].reg_sp = (memaddr) addr;
    sstStates[asid - 1].status = ALLOFF | IEPON | IMON | TEBITON;
    sstStates[asid - 1].entry_hi = asid << ASIDSHIFT;
    addr -= PAGESIZE;
    // init support
    // TODO: possibile errore in stckPtr -> guardare ultimo punto della sez 10
    supports[asid - 1].sup_asid = asid;
    supports[asid - 1].sup_exceptContext[PGFAULTEXCEPT].stackPtr = (memaddr) addr;
    supports[asid - 1].sup_exceptContext[PGFAULTEXCEPT].status = ALLOFF | IEPON | IMON | TEBITON;
    supports[asid - 1].sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr) TLB_ExceptionHandler;
    addr -= PAGESIZE;
    supports[asid - 1].sup_exceptContext[GENERALEXCEPT].stackPtr = (memaddr) addr;
    supports[asid - 1].sup_exceptContext[GENERALEXCEPT].status = ALLOFF | IEPON | IMON | TEBITON;
    supports[asid - 1].sup_exceptContext[GENERALEXCEPT].pc = (memaddr) supportExceptionHandler;
    addr -= PAGESIZE;
    for (int i = 0; i < USERPGTBLSIZE; i++) {
      initPageTableEntry(asid, &(supports[asid - 1].sup_privatePgTbl[i]), i);
    }

    // create sst process
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
  }
}

void initPageTableEntry(unsigned int asid, pteEntry_t *entry, int idx){
  // Setto tutti i bit a 0
  entry->pte_entryHI &= 0x0;

  // Setto il VPN
  if (idx < 31){
    entry->pte_entryHI |= KUSEG;
    entry->pte_entryHI |= (idx << VPNSHIFT);
  }
  else {
    entry->pte_entryHI |= 0xBFFFF000;
  }

  // Setto l'asid
  entry->pte_entryHI |= (asid << ASIDSHIFT);

  entry->pte_entryLO &= 0x0;

  // Setto D = 1
  entry->pte_entryLO |= DIRTYON;

}

void initSwapMutex() {
  swapMutexState.reg_sp = (memaddr) addr;
  swapMutexState.pc_epc = swapMutexState.reg_t9 = (memaddr) swapMutex;
  swapMutexState.status = ALLOFF | IEPON | IMON | TEBITON;

  addr -= PAGESIZE;

  pcb_PTR p;
  ssi_create_process_t create = {
      .state = &swapMutexState,
      .support = NULL,
  };
  ssi_payload_t createPayload = {
      .service_code = CREATEPROCESS,
      .arg = &create,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &createPayload, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &p, 0);
  
  swapMutexProcess = p;
}

void swapMutex() {
  while(TRUE) {
    unsigned int sender = SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
    mutexHolderProcess = (pcb_t *)sender;
    SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
    SYSCALL(RECEIVEMESSAGE, sender, 0, 0);
    mutexHolderProcess = NULL;
  }
}