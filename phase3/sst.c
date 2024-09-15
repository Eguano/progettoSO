#include "sst.h"

extern pcb_PTR test_pcb;
extern pcb_PTR ssi_pcb;
extern state_t uprocStates[UPROCMAX];
extern swpo_t swap_pool[POOLSIZE];

extern unsigned int debug;

/**
 * Richiede all'SSI la struttura di supporto del processo SST
 * 
 * @return puntatore alla struttura di supporto
 */
support_t *getSupport() {
  support_t *sup;
  ssi_payload_t payload = {
    .service_code = GETSUPPORTPTR,
    .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &payload, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &sup, 0);
  return sup;
}

/**
 * Crea un U-proc figlio poi si mette in ascolto di richieste
 * 
 * @param s puntatore allo state dell'U-proc da creare
 */
void SSTInitialize() {
  // child initialization
  // richiede la struttura di supporto
  support_t *sup;
  ssi_payload_t payload_sup= {
  .service_code = GETSUPPORTPTR,
  .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &payload_sup, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &sup, 0);

  // crea U-proc figlio
  pcb_PTR p;
  ssi_create_process_t createProcess = {
    .state = &uprocStates[sup->sup_asid - 1],
    .support = sup,
  };
  ssi_payload_t payload = {
    .service_code = CREATEPROCESS,
    .arg = &createProcess,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &payload, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &p, 0);

  SSTHandler(sup->sup_asid);
}

/**
 * Gestisce una richiesta ricevuta da un processo utente
 * 
 * @param asid asid dell'U-proc figlio dell'SST
 */
void SSTHandler(int asid) {
  while (TRUE) {
    debug = 0x300;
    ssi_payload_PTR p_payload = NULL;
    pcb_PTR sender = (pcb_PTR) SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int) &p_payload, 0);
    // risposta da inviare all'U-proc
    unsigned int response = 0;
    debug = 0x301;

    // accoglimento richiesta
    switch(p_payload->service_code) {
      case GET_TOD:
        // restituire TOD
        STCK(response);
        break;
      case TERMINATE:
        // terminare SST e di conseguenza l'U-proc
        terminate(asid);
        break;
      case WRITEPRINTER:
        // scrivere una stringa su una printer
        writePrinter(asid, (sst_print_PTR) p_payload->arg);
        break;
      case WRITETERMINAL:
        // scrivere una stringa su un terminal
        writeTerminal(asid, (sst_print_PTR) p_payload->arg);
        break;
      default:
        // errore
        break;
    }

    debug = 0x302;
    SYSCALL(SENDMESSAGE, (unsigned int) sender, response, 0);
  }
}

/**
 * Termina il processo corrente
 */
void terminate(int asid) {
  // se l'U-proc occupa dei frame sono da liberare
  for (int i = 0; i < POOLSIZE; i++) {
    if (swap_pool[i].swpo_asid == asid) {
      swap_pool[i].swpo_asid = NOPROC;
    }
  }
  // comunica al test la terminazione
  SYSCALL(SENDMESSAGE, (unsigned int) test_pcb, 0, 0);
  // vera terminazione
  ssi_payload_t termPayload = {
    .service_code = TERMPROCESS,
    .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &termPayload, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, 0, 0);
}

/**
 * Scrive una stringa di caratteri su una printer
 * 
 * @param asid asid del processo che richiede la stampa
 * @param arg payload con stringa da stampare e lunghezza
 */
void writePrinter(int asid, sst_print_PTR arg) {
  // individua l'istanza di printer su cui scrivere
  dtpreg_t *base = (dtpreg_t *)DEV_REG_ADDR(PRNTINT, asid - 1);
  unsigned int *data0 = &base->data0;
  unsigned int status;
  // stringa da printare
  char *s = arg->string;

  while (*s != EOS) {
    unsigned int value = PRINTCHR;
    *data0 = (unsigned int) *s;
    ssi_do_io_t doIO = {
      .commandAddr = &base->command,
      .commandValue = value,
    };
    ssi_payload_t ioPayload = {
      .service_code = DOIO,
      .arg = &doIO,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int) &ioPayload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int) &status, 0);

    // verifica la correttezza dell'operazione
    if (status != READY) {
      debug = 0x7;
      PANIC();
    }

    s++;
  }
}

/**
 * Scrive una stringa di caratteri su un terminal
 * 
 * @param asid asid del processo che richiede la stampa
 * @param arg payload con stringa da stampare e lunghezza
 */
void writeTerminal(int asid, sst_print_PTR arg) {
  // individua l'istanza di terminal su cui scrivere
  termreg_t *base = (termreg_t *)DEV_REG_ADDR(TERMINT, asid - 1);
  unsigned int status;
  // stringa da printare
  char *s = arg->string;

  while (*s != EOS) {
    unsigned int value = PRINTCHR | (((unsigned int) *s) << 8);
    ssi_do_io_t doIO = {
      .commandAddr = &base->transm_command,
      .commandValue = value,
    };
    ssi_payload_t ioPayload = {
      .service_code = DOIO,
      .arg = &doIO,
    };
    debug = &doIO.commandAddr;
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int) &ioPayload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int) &status, 0);

    // controlla la correttezza dell'operazione
    if ((status & TERMSTATMASK) != RECVD) {
      debug = 0x8;
      PANIC();
    }

    s++;
  }
}