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
  SYSCALL(SENDMESSAGE, ssiAddress, &payload, 0);
  // se non è OK il processo ssi non esiste -> emergency shutdown
  if (sender->p_s.reg_v0 != OK) PANIC();
  SYSCALL(RECEIVEMESSAGE, ssiAddress, sender, 0);
}

/**
 * Gestisce una richiesta ricevuta da un processo
 */
void SSIHandler() {
  while (TRUE) {
    ssi_payload_t payload;
    SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, &payload, 0);
    // processo mittente
    pcb_PTR sender = currentProcess->p_s.reg_v0;
    // risposta da inviare al sender
    unsigned int response = NULL;
    // esecuzione del servizio richiesto
    if (payload.service_code == CREATEPROCESS) {
      // creare un nuovo processo
      response = createProcess((ssi_create_process_PTR) payload.arg, sender);
    } else if (payload.service_code == TERMPROCESS) {
      // eliminare un processo esistente
      if (payload.arg == NULL) {
        terminateProcess(sender);
      } else {
        terminateProcess(payload.arg);
      }
    } else if (payload.service_code == DOIO) {
      // eseguire un input o output
      // TODO: fare sezione 7.3, necessari interrupt
    } else if (payload.service_code == GETTIME) {
      // restituire accumulated processor time
      response = &(sender->p_time);
    } else if (payload.service_code == CLOCKWAIT) {
      // bloccare il processo per lo pseudoclock
      insertProcQ(&pseudoclockBlocked->p_list, sender);
      waitingCount++;
      /* TODO: quando un processo è in attesa della receive, dove viene bloccato?
      quando gli rispondo dove finisce? verificare se è da eliminare da altre liste*/
    } else if (payload.service_code == GETSUPPORTPTR) {
      // restituire la struttura di supporto
      response = sender->p_supportStruct;
    } else if (payload.service_code == GETPROCESSID) {
      // restituire il pid del sender o del suo genitore
      if (payload.arg == 0) {
        response = sender->p_pid;
      } else {
        if (sender->p_parent == NULL) {
          response = 0;
        } else {
          response = sender->p_parent->p_pid;
        }
      }
    } else {
      // codice non esiste, terminare processo richiedente e tutta la sua progenie
      terminateProcess(sender);
    }
    SYSCALL(SENDMESSAGE, sender, response, 0);
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
  outChild(proc);
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