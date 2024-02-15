#include "ssi.h"

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
  // SYSCALL(SENDMESSAGE, (memaddr) ssi, &payload, 0);
  if (sender->p_s.reg_v0 != OK) PANIC();
  // SYSCALL(RECEIVEMESSAGE, (memaddr) ssi, sender, 0);
}

/**
 * Gestisce una richiesta ricevuta da un processo
 */
void SSIHandler() {
  while (TRUE) {
    ssi_payload_t payload;
    SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, &payload, 0);
    pcb_PTR sender = currentProcess->p_s.reg_v0;
    unsigned int result = NULL;
    if (payload.service_code == CREATEPROCESS) {
      // creare un nuovo processo
      result = createProcess((ssi_create_process_PTR) payload.arg, sender);
    } else if (payload.service_code == TERMPROCESS) {
      // eliminare un processo esistente
      if (payload.arg == NULL) {
        terminateProcess(sender);
      } else {
        terminateProcess(payload.arg);
      }
    } else if (payload.service_code == DOIO) {
      // TODO: fare sezione 7.3
    } else if (payload.service_code == GETTIME) {
      // restituire accumulated processor time
      result = &(sender->p_time);
    } else if (payload.service_code == CLOCKWAIT) {
      // TODO: fare sezione 7.5
    } else if (payload.service_code == GETSUPPORTPTR) {
      // restituire la struttura di supporto
      result = sender->p_supportStruct;
    } else if (payload.service_code == GETPROCESSID) {
      // restituire il pid del sender o del suo genitore
      if (payload.arg == 0) {
        result = sender->p_pid;
      } else {
        if (sender->p_parent == NULL) {
          result = 0;
        } else {
          result = sender->p_parent->p_pid;
        }
      }
    } else {
      // codice non esiste, terminare processo richiedente e tutta la sua progenie
      terminateProcess(sender);
    }
    SYSCALL(SENDMESSAGE, sender, result, 0);
  }
}

/**
 * Crea un processo come figlio del richiedente e lo posiziona in readyQueue
 * 
 * @param arg struttura contenente state e support (eventuale)
 * @param sender processo richiedente
 * @return puntatore al processo creato, NOPROC altrimenti
 */
unsigned int createProcess(ssi_create_process_t *arg, pcb_t *sender) {
  pcb_PTR p = allocPcb();
  if (p == NULL) {
    return NOPROC;
  } else {
    p->p_s = *arg->state;
    p->p_supportStruct = arg->support;
    insertProcQ(&readyQueue->p_list, p);
    processCount++;
    insertChild(sender, p);
    return p;
  }
}

/**
 * Elimina un processo e tutta la sua progenie
 * 
 * @param proc processo da eliminare
 */
void terminateProcess(pcb_t *proc) {
  proc = outChild(proc);
  if (!emptyChild(proc)) terminateProgeny(proc);
  destroyProcess(proc);
}

/**
 * Elimina tutti i processi figli di p
 * 
 * @param p processo di cui eliminare i figli
 */
void terminateProgeny(pcb_t *p) {
  while (!emptyChild(p)) {
    terminateProgeny(getFirstChild(p));
    pcb_PTR removed = removeChild(p);
    destroyProcess(removed);
  }
}

/**
 * Elimina il processo p dal sistema e lo ripone nella lista dei free pcb
 * 
 * @param p processo da rimuovere dalle code
 */
void destroyProcess(pcb_t *p) {
  // lo cerco nella ready queue
  if (outProcQ(&readyQueue->p_list, p) == NULL) {
    // se non è nella ready lo cerco tra i bloccati per lo pseudoclock
    if (outProcQ(&pseudoclockBlocked->p_list, p) == NULL) {
      // se non è per lo pseudoclock, è bloccato per un altro interrupt
      int found = 0;
      for (int i = 0; i < SEMDEVLEN - 1 && found == 0; i++) {
        if (outProcQ(&externalBlocked[i]->p_list, p) != NULL) found = 1;
      }
      for (int i = 0; i < MAXTERMINALDEV && found == 0; i++) {
        if (outProcQ(&terminalBlocked[0][i]->p_list, p) != NULL) found = 1;
        if (outProcQ(&terminalBlocked[1][i]->p_list, p) != NULL) found = 1;
      }
    }
    waitingCount--;
  }
  freePcb(p);
  processCount--;
}