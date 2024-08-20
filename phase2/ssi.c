#include "ssi.h"

#include "../phase1/headers/pcb.h"

extern int process_count;
extern int waiting_count;
extern pcb_PTR current_process;
extern struct list_head ready_queue;
extern struct list_head external_blocked_list[4][MAXDEV];
extern struct list_head pseudoclock_blocked_list;
extern struct list_head terminal_blocked_list[2][MAXDEV];
extern void copyRegisters(state_t *dest, state_t *src);

/**
 * Gestisce una richiesta ricevuta da un processo
 */
void SSIHandler() {
  while (TRUE) {
    ssi_payload_PTR p_payload;
    pcb_PTR sender = (pcb_PTR) SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int) &p_payload, 0);
    // risposta da inviare al sender
    unsigned int response = 0;

    // esecuzione del servizio richiesto
    switch (p_payload->service_code) {
      case CREATEPROCESS:
        // creare un nuovo processo
        response = createProcess((ssi_create_process_PTR) p_payload->arg, sender);
        break;
      case TERMPROCESS:
        // eliminare un processo esistente
        if (p_payload->arg == NULL) {
          terminateProcess(sender);
        } else {
          terminateProcess((pcb_PTR) p_payload->arg);
        }
        break;
      case DOIO:
        // eseguire un input o output
        blockForDevice((ssi_do_io_PTR) p_payload->arg, sender);
        break;
      case GETTIME:
        // restituire accumulated processor time
        response = ((unsigned int) sender->p_time) + (TIMESLICE - getTIMER());
        break;
      case CLOCKWAIT:
        // bloccare il processo per lo pseudoclock
        insertProcQ(&pseudoclock_blocked_list, sender);
        waiting_count++;
        break;
      case GETSUPPORTPTR:
        // restituire la struttura di supporto
        response = (unsigned int) sender->p_supportStruct;
        break;
      case GETPROCESSID:
        // restituire il pid del sender o del suo genitore
        if (((unsigned int) p_payload->arg) == 0) {
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
        // terminare operazione IO
        response = sender->p_s.reg_v0;
        break;
      default:
        // codice non esiste, terminare processo richiedente e tutta la sua progenie
        terminateProcess(sender);
        break;
    }
    if (p_payload->service_code != DOIO) {
      SYSCALL(SENDMESSAGE, (unsigned int) sender, response, 0);
    }
  }
}

/**
 * Crea un processo come figlio del richiedente e lo posiziona in readyQueue
 * 
 * @param arg struttura contenente state e (eventuale) support
 * @param sender processo richiedente
 * @return puntatore al processo creato, NOPROC altrimenti
 */
static unsigned int createProcess(ssi_create_process_t *arg, pcb_t *sender) {
  pcb_PTR p = allocPcb();
  if (p == NULL) {
    return (unsigned int) NOPROC;
  } else {
    copyRegisters(&p->p_s, arg->state);
    if (arg->support != NULL) p->p_supportStruct = arg->support;
    insertChild(sender, p);
    insertProcQ(&ready_queue, p);
    process_count++;
    return (unsigned int) p;
  }
}

/**
 * Elimina un processo e tutta la sua progenie
 * 
 * @param p processo da eliminare
 */
void terminateProcess(pcb_t *p) {
  outChild(p);
  terminateProgeny(p);
  destroyProcess(p);
}

/**
 * Elimina tutti i processi figli di p
 * 
 * @param p processo di cui eliminare i figli
 */
void terminateProgeny(pcb_t *p) {
  while (!emptyChild(p)) {
    // rimuove il primo figlio
    pcb_t *child = removeChild(p);
    // se è stato rimosso con successo, elimina ricorsivamente i suoi figli
    if (child != NULL) {
      terminateProgeny(child);
      // dopo aver eliminato i figli, distrugge il processo
      destroyProcess(child);
    }
  }
}

/**
 * Elimina il processo p dal sistema e lo ripone nella lista dei free pcb
 * 
 * @param p processo da rimuovere dalle code
 */
void destroyProcess(pcb_t *p) {
  if (!isInPCBFree_h(p)) {
    // lo cerco nella ready queue
    if (outProcQ(&ready_queue, p) == NULL) {
      // se non è nella ready lo cerco tra i bloccati per lo pseudoclock
      int found = FALSE;
      if (outProcQ(&pseudoclock_blocked_list, p) == NULL) {
        // se non è per lo pseudoclock, magari è bloccato per un altro interrupt
        for (int i = 0; i < MAXDEV && found == FALSE; i++) {
          if (outProcQ(&external_blocked_list[0][i], p) != NULL) found = TRUE;
          if (outProcQ(&external_blocked_list[1][i], p) != NULL) found = TRUE;
          if (outProcQ(&external_blocked_list[2][i], p) != NULL) found = TRUE;
          if (outProcQ(&external_blocked_list[3][i], p) != NULL) found = TRUE;
          if (outProcQ(&terminal_blocked_list[0][i], p) != NULL) found = TRUE;
          if (outProcQ(&terminal_blocked_list[1][i], p) != NULL) found = TRUE;
        }
      } else {
        found = TRUE;
      }
      // contatore diminuito solo se era bloccato per DOIO o PseudoClock
      if (found) waiting_count--;
    }
    freePcb(p);
    process_count--;
  }
}

/**
 * Blocca il processo nella lista del device per cui ha richiesto un'operazione
 * 
 * @param arg puntatore alla struttura di DOIO
 * @param toBlock processo da bloccare
 */
static void blockForDevice(ssi_do_io_t *arg, pcb_t *toBlock) {
  int instance;
  // per sicurezza controlla che non sia in ready queue
  outProcQ(&ready_queue, toBlock);
  // calcola l'indirizzo base del registro
  memaddr devRegAddr = (memaddr) arg->commandAddr - 0x4;
  switch (devRegAddr) {
    case START_DEVREG ... 0x100000C4:
      // disks
      instance = (devRegAddr - START_DEVREG) / 0x00000010;
      insertProcQ(&external_blocked_list[0][instance], toBlock);
      break;
    case 0x100000D4 ... 0x10000144:
      // flash
      instance = (devRegAddr - 0x100000D4) / 0x00000010;
      insertProcQ(&external_blocked_list[1][instance], toBlock);
      break;
    case 0x10000154 ... 0x100001C4:
      // network
      instance = (devRegAddr - 0x10000154) / 0x00000010;
      insertProcQ(&external_blocked_list[2][instance], toBlock);
      break;
    case 0x100001D4 ... 0x10000244:
      // printer
      instance = (devRegAddr - 0x100001D4) / 0x00000010;
      insertProcQ(&external_blocked_list[3][instance], toBlock);
      break;
    case 0x10000254 ... 0x100002C4:
      // terminal
      // TODO: per ora si usa solo il terminale 0 quindi rimando i controlli sulla instance a più avanti
      if (devRegAddr == 0x10000254 || devRegAddr == 0x10000264 || devRegAddr == 0x10000274
      || devRegAddr == 0x10000284 || devRegAddr == 0x10000294  || devRegAddr == 0x100002A4
      || devRegAddr == 0x100002B4 || devRegAddr == 0x100002C4) {
        // receiver
        instance = 0;
        insertProcQ(&terminal_blocked_list[1][instance], toBlock);
      } else {
        // transmitter
        instance = 0;
        insertProcQ(&terminal_blocked_list[0][instance], toBlock);
      }
      break;
    default:
      // error
      break;
  }
  waiting_count++;
  *arg->commandAddr = arg->commandValue;
}