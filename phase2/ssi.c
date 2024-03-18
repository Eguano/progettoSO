#include "ssi.h"

#include "../phase1/headers/pcb.h"

extern int process_count;
extern int waiting_count;
extern pcb_PTR current_process;
extern pcb_PTR ready_queue;
extern pcb_PTR external_blocked_list[4][MAXDEV];
extern pcb_PTR pseudoclock_blocked_list;
extern pcb_PTR terminal_blocked_list[2][MAXDEV];
extern pcb_PTR ssi_pcb;

/**
 * Invia una richiesta all'SSI
 * 
 * @param sender area dove salvare la risposta
 * @param service codice per il servizio richiesto
 * @param arg parametro per il servizio (se necessario)
 */
void SSIRequest(pcb_t* sender, int service, void* arg) {
  ssi_payload_t payload = {service, arg};
  SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int) &payload, 0);
  // se non è OK il processo ssi non esiste -> emergency shutdown
  if (sender->p_s.reg_v0 != OK) PANIC();
  SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int) sender, 0);
}

/**
 * Gestisce una richiesta ricevuta da un processo
 */
void SSIHandler() {
  while (TRUE) {
    ssi_payload_t payload;
    SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int) &payload, 0);
    // processo mittente
    pcb_PTR sender = (pcb_PTR) current_process->p_s.reg_v0;
    // risposta da inviare al sender
    unsigned int response = (unsigned int) NULL;

    // esecuzione del servizio richiesto
    if (payload.service_code == CREATEPROCESS) {
      // creare un nuovo processo
      // TODO: gestire il return NULL (se non alloca il processo) -> bisogna rispondere NOPROC
      response = (unsigned int) createProcess((ssi_create_process_PTR) payload.arg, sender);
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
      response = (unsigned int) &(sender->p_time);
    } else if (payload.service_code == CLOCKWAIT) {
      // bloccare il processo per lo pseudoclock
      insertProcQ(&pseudoclock_blocked_list->p_list, sender);
      waiting_count++;
      /* TODO: quando un processo è in attesa della receive, dove viene bloccato?
      quando gli rispondo dove finisce? verificare se è da eliminare da altre liste*/
    } else if (payload.service_code == GETSUPPORTPTR) {
      // restituire la struttura di supporto
      response = (unsigned int) sender->p_supportStruct;
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
    SYSCALL(SENDMESSAGE, (unsigned int) sender, response, 0);
  }
}

/**
 * Crea un processo come figlio del richiedente e lo posiziona in readyQueue
 * 
 * @param arg struttura contenente state e support (eventuale)
 * @param sender processo richiedente
 * @return puntatore al processo creato, NULL altrimenti
 */
static pcb_PTR createProcess(ssi_create_process_t *arg, pcb_t *sender) {
  pcb_PTR p = allocPcb();
  if (p != NULL) {
    p->p_s = *arg->state;
    p->p_supportStruct = arg->support;
    insertProcQ(&ready_queue->p_list, p);
    process_count++;
    insertChild(sender, p);
  }
  return p;
}

/**
 * Elimina un processo e tutta la sua progenie
 * 
 * @param proc processo da eliminare
 */
static void terminateProcess(pcb_t *proc) {
  outChild(proc);
  if (!emptyChild(proc)) terminateProgeny(proc);
  destroyProcess(proc);
}

/**
 * Elimina tutti i processi figli di p
 * 
 * @param p processo di cui eliminare i figli
 */
static void terminateProgeny(pcb_t *p) {
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
static void destroyProcess(pcb_t *p) {
  // lo cerco nella ready queue
  if (outProcQ(&ready_queue->p_list, p) == NULL) {
    // se non è nella ready lo cerco tra i bloccati per lo pseudoclock
    if (outProcQ(&pseudoclock_blocked_list->p_list, p) == NULL) {
      // se non è per lo pseudoclock, è bloccato per un altro interrupt
      int found = 0;
      for (int i = 0; i < MAXDEV && found == 0; i++) {
        if (outProcQ(&external_blocked_list[0][i]->p_list, p) != NULL) found = 1;
        if (outProcQ(&external_blocked_list[1][i]->p_list, p) != NULL) found = 1;
        if (outProcQ(&external_blocked_list[2][i]->p_list, p) != NULL) found = 1;
        if (outProcQ(&external_blocked_list[3][i]->p_list, p) != NULL) found = 1;
        if (outProcQ(&terminal_blocked_list[0][i]->p_list, p) != NULL) found = 1;
        if (outProcQ(&terminal_blocked_list[1][i]->p_list, p) != NULL) found = 1;
      }
    }
    waiting_count--;
  }
  freePcb(p);
  process_count--;
}