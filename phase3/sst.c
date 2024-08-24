#include "sst.h"

extern pcb_PTR test_pcb;
extern pcb_PTR ssi_pcb;

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
void SSTInitialize(state_t *s) {
  // child initialization
  // richiede la struttura di supporto
  support_t *sup = getSupport();
  // crea U-proc figlio
  pcb_PTR p;
  ssi_create_process_t createProcess = {
    .state = s,
    .support = sup,
  };
  ssi_payload_t payload = {
    .service_code = CREATEPROCESS,
    .arg = &createProcess,
  };
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &payload, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &p, 0);

  SSTHandler();
}

/**
 * Gestisce una richiesta ricevuta da un processo utente
 */
void SSTHandler() {
  while (TRUE) {
    ssi_payload_PTR p_payload;
    pcb_PTR sender = (pcb_PTR) SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int) p_payload, 0);
    // risposta da inviare all'U-proc
    unsigned int response = 0;

    // accoglimento richiesta
    switch(p_payload->service_code) {
      case GET_TOD:
        // restituire TOD
        STCK(response);
        break;
      case TERMINATE:
        // terminare SST e di conseguenza l'U-proc
        // comunica al test la terminazione
        SYSCALL(SENDMESSAGE, (unsigned int) test_pcb, 0, 0);
        // vera terminazione
        ssi_payload_t termPayload = {
          .service_code = TERMPROCESS,
          .arg = NULL,
        };
        SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &termPayload, 0);
        SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, 0, 0);
        break;
      case WRITEPRINTER:
        // scrivere una stringa su una printer
        write(((sst_print_PTR) p_payload->arg)->string);
        break;
      case WRITETERMINAL:
        // scrivere una stringa su un terminal
        write(((sst_print_PTR) p_payload->arg)->string);
        break;
      default:
        // errore?
        break;
    }

    SYSCALL(SENDMESSAGE, (unsigned int) sender, response, 0);
  }
}

void write(char* s) {
  // TODO: individuare device su cui scrivere
  unsigned int *base = (unsigned int *)(0x10000254);
  unsigned int *command = base + 3;
  unsigned int status;

  while (*s != EOS) {
    unsigned int value = PRINTCHR | (((unsigned int)*s) << 8);
    ssi_do_io_t do_io = {
      .commandAddr = command,
      .commandValue = value,
    };
    ssi_payload_t payload = {
      .service_code = DOIO,
      .arg = &do_io,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status), 0);

    if ((status & 0xFF) != RECVD) {
      PANIC();
    }

    s++;
  }
}