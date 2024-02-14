#include "ssi.h"

// TODO: if the ssi gets terminated -> emergency shutdown

/**
 * Invia una richiesta all'SSI
 * 
 * @param sender area dove salvare la risposta
 * @param service codice per il servizio richiesto
 * @param arg parametro per il servizio (se necessario)
 */
void SSIRequest(pcb_t* sender, int service, void* arg) {
  ssi_payload_t payload = {service, arg};
  // TODO: gestire indirizzo del processo SSI
  /* SYSCALL(SENDMESSAGE, (memaddr) ssi, &payload, 0);
  se è tutto apposto (verificare v0) effettuare la receive,
  altrimenti emergency shutdown
  SYSCALL(RECEIVEMESSAGE, (memaddr) ssi, sender, 0); */
}

/**
 * Gestisce una richiesta ricevuta da un processo
 */
void SSIHandler() {
  while (TRUE) {
    ssi_payload_t payload;
    SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, &payload, 0);
    if (payload.service_code == CREATEPROCESS) {
      
    } else if (payload.service_code == TERMPROCESS) {
      
    } else if (payload.service_code == DOIO) {
      
    } else if (payload.service_code == GETTIME) {
      
    } else if (payload.service_code == CLOCKWAIT) {
      
    } else if (payload.service_code == GETSUPPORTPTR) {
      
    } else if (payload.service_code == GETPROCESSID) {
      
    } else {
      // codice non esiste, teminare processo richiedente e tutta la sua progenie
    }
    // SYSCALL(SENDMESSAGE, currentProcess->p_s.reg_v0, result, 0);
  }
}