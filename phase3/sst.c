#include "sst.h"

extern pcb_PTR test_pcb;
extern pcb_PTR ssi_pcb;
extern state_t uprocStates[UPROCMAX];
extern swpo_t swap_pool[POOLSIZE];

/**
 * Crea un U-proc figlio poi si mette in ascolto di richieste
 * 
 * @param s puntatore allo state dell'U-proc da creare
 */
void SSTInitialize() {
  // Richiedo la struttura di supporto alla SSI
  support_t *sup;
  ssi_payload_t payload_sup= {
  .service_code = GETSUPPORTPTR,
  .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &payload_sup, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &sup, 0);

  // Creo l'U-proc figlio facendo una richiesta alla SSI
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

  // Invoco l'SST handler
  SSTHandler(sup->sup_asid);
}

/**
 * Gestisce una richiesta ricevuta da un processo utente
 * 
 * @param asid asid dell'U-proc figlio dell'SST
 */
void SSTHandler(int asid) {
  while (TRUE) {
    ssi_payload_PTR p_payload = NULL;
    // Mi metto in ascolto della richiesta da gestire
    pcb_PTR sender = (pcb_PTR) SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int) &p_payload, 0);
    // Risposta da inviare all'U-proc
    unsigned int response = 0;

    // Gestisco la richiesta
    switch(p_payload->service_code) {
      case GET_TOD:
        // Restituire TOD
        STCK(response);
        break;
      case TERMINATE:
        // Terminare SST e di conseguenza l'U-proc
        terminate(asid);
        break;
      case WRITEPRINTER:
        // Scrivere una stringa su una printer
        writePrinter(asid, (sst_print_PTR) p_payload->arg);
        break;
      case WRITETERMINAL:
        // Scrivere una stringa su un terminal
        writeTerminal(asid, (sst_print_PTR) p_payload->arg);
        break;
      default:
        // Errore
        break;
    }

    // Mando la risposta al processo che ha fatto la richiesta
    SYSCALL(SENDMESSAGE, (unsigned int) sender, response, 0);
  }
}

/**
 * Termina il processo corrente
 */
void terminate(int asid) {
  // Libero i frame occupati dall'U-proc
  for (int i = 0; i < POOLSIZE; i++) {
    if (swap_pool[i].swpo_asid == asid) {
      swap_pool[i].swpo_asid = NOPROC;
    }
  }
  // Comunica al test la terminazione del processo
  SYSCALL(SENDMESSAGE, (unsigned int) test_pcb, 0, 0);
  
  // Effettuo la terminazione con una richiesta alla SSI
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
 * @param arg payload con stringa da stampare e la sua lunghezza
 */
void writePrinter(int asid, sst_print_PTR arg) {
  // Prendo il registro della printer da utilizzare
  dtpreg_t *base = (dtpreg_t *)DEV_REG_ADDR(PRNTINT, asid - 1);
  
  // Valore di ritorno dell'operazione
  unsigned int status;
  
  // Stringa da stampare
  char *s = arg->string;

  // Manda un messaggio per carattere
  while (*s != EOS) {
    // Inserisco il carattere in data0
    base->data0 = (unsigned int) *s;
    
    // Faccio una richiesta DOIO alla SSI
    ssi_do_io_t doIO = {
      .commandAddr = &base->command,
      .commandValue = PRINTCHR,
    };
    ssi_payload_t ioPayload = {
      .service_code = DOIO,
      .arg = &doIO,
    };

    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int) &ioPayload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int) &status, 0);

    // Verifica la correttezza dell'operazione
    if (status != READY) {
      PANIC();
    }

    s++;
  }
}

/**
 * Scrive una stringa di caratteri su un terminal
 * 
 * @param asid asid del processo che richiede la stampa
 * @param arg payload con stringa da stampare e la sua lunghezza
 */
void writeTerminal(int asid, sst_print_PTR arg) {
  // Prendo il registro del terminale da utilizzare
  termreg_t *base = (termreg_t *)DEV_REG_ADDR(TERMINT, asid - 1);
  
  // Valore di ritorno dell'operazione
  unsigned int status;
  
  // Stringa da stampare
  char *s = arg->string;

  // Manda un messaggio per carattere
  while (*s != EOS) {

    // Faccio una richiesta DOIO alla SSI
    ssi_do_io_t doIO = {
      .commandAddr = &base->transm_command,
      .commandValue = PRINTCHR | (((unsigned int) *s) << 8),
    };
    ssi_payload_t ioPayload = {
      .service_code = DOIO,
      .arg = &doIO,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int) &ioPayload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int) &status, 0);

    // Verifica la correttezza dell'operazione
    if ((status & TERMSTATMASK) != RECVD) {
      PANIC();
    }

    s++;
  }
}