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
    pcb_PTR sender = (pcb_PTR) SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int) &payload, 0);
    // risposta da inviare al sender
    unsigned int response = (unsigned int) NULL;

    // esecuzione del servizio richiesto
    switch (payload.service_code) {
      case CREATEPROCESS:
        // creare un nuovo processo
        // TODO: gestire il return NULL (se non alloca il processo) -> bisogna rispondere NOPROC
        response = (unsigned int) createProcess((ssi_create_process_PTR) payload.arg, sender);
        process_count++;
        break;
      case TERMPROCESS:
        // eliminare un processo esistente
        if (payload.arg == NULL) {
          terminateProcess(sender);
        } else {
          terminateProcess(payload.arg);
        }
        break;
      case DOIO:
        // eseguire un input o output
        int dev = findDevice(((ssi_do_io_PTR) payload.arg)->commandAddr) / 10;
        int devInstance = findDevice(((ssi_do_io_PTR) payload.arg)->commandAddr) % 10;
        if (dev == 4) {
          // dispositivo terminale receiver
          insertProcQ(&terminal_blocked_list[1][devInstance]->p_list, sender);
        } else if (dev == 5) {
          // dispositivo terminale transmitter
          insertProcQ(&terminal_blocked_list[0][devInstance]->p_list, sender);
        } else {
          // dispositivo periferico
          insertProcQ(&external_blocked_list[dev][devInstance]->p_list, sender);
        }
        waiting_count++;
        *((ssi_do_io_PTR) payload.arg)->commandAddr = ((ssi_do_io_PTR) payload.arg)->commandValue;
        // TODO: interruptHandler deve mandare un messaggio con il risultato dell'operazione
        break;
      case GETTIME:
        // restituire accumulated processor time
        response = (unsigned int) &(sender->p_time);
        break;
      case CLOCKWAIT:
        // bloccare il processo per lo pseudoclock
        insertProcQ(&pseudoclock_blocked_list->p_list, sender);
        waiting_count++;
        break;
      case GETSUPPORTPTR:
        // restituire la struttura di supporto
        response = (unsigned int) sender->p_supportStruct;
        break;
      case GETPROCESSID:
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
        break;
      case ENDIO:
        // terminazione operazione IO
        sender = ((ssi_end_io_PTR) payload.arg)->toUnblock;
        response = ((ssi_end_io_PTR) payload.arg)->status;
        break;
      default:
        // codice non esiste, terminare processo richiedente e tutta la sua progenie
        terminateProcess(sender);
        break;
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
  if (!isInPCBFree_h(p)) {
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
}

static int findDevice(memaddr *commandAddr) {
  memaddr devRegAddr = *commandAddr - 0x4;
  switch (devRegAddr) {
    case START_DEVREG ... 0x100000C4:
      return findInstance(devRegAddr - START_DEVREG);
    case 0x100000D4 ... 0x10000144:
      return 10 + findInstance(devRegAddr - 0x100000D4);
    case 0x10000154 ... 0x100001C4:
      return 20 + findInstance(devRegAddr - 0x10000154);
    case 0x100001D4 ... 0x10000244:
      return 30 + findInstance(devRegAddr - 0x100001D4);
    case 0x10000254 ... 0x100002C4:
      // TODO: per ora si usa solo il terminale 0 quindi rimando i controlli a più avanti per gli altri
      if (devRegAddr == 0x10000254 || devRegAddr == 0x10000264 || devRegAddr == 0x10000274
      || devRegAddr == 0x10000284 || devRegAddr == 0x10000294  || devRegAddr == 0x100002A4
      || devRegAddr == 0x100002B4 || devRegAddr == 0x100002C4) {
        // receiver
        return 40;
      } else {
        return 50;
      }
      break;
    default:
      return 0;
      break;
  }
}

static int findInstance(memaddr distFromBase) {
  return distFromBase / 0x00000010;
}