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
extern unsigned int debug;

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
        response = (unsigned int) sender->p_time;
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
  
  // ciclo for che itera fra i dispositvi terminali
  for (int dev = 0; dev < 8; dev++) {
    // calcolo indirizzo di base
    termreg_t *base_address = (termreg_t *)DEV_REG_ADDR(7, dev);
    if (arg->commandAddr == (memaddr) & (base_address->recv_command)) {
      // inizializzazione del campo aggiuntivo del pcb
      insertProcQ(&terminal_blocked_list[1][dev], toBlock);
      waiting_count++;
      *arg->commandAddr = arg->commandValue;
      return;
    } else if (arg->commandAddr == (memaddr) & (base_address->transm_command)) {
      // inizializzazione del campo aggiuntivo del pcb
      insertProcQ(&terminal_blocked_list[0][dev], toBlock);
      waiting_count++;
      *arg->commandAddr = arg->commandValue;
      return;
    }
  }
  // ciclo for che itera fra tutti gli altri dispositivi
  for (int line = 3; line < 7; line++) {
    for (int dev = 0; dev < 8; dev++) {
      dtpreg_t *base_address = (dtpreg_t *)DEV_REG_ADDR(line, dev);
      if (arg->commandAddr == (memaddr) & (base_address->command)) {
        // inizializzazione del campo aggiuntivo del pcb
        switch (line) {
          case IL_DISK:
            debug = 0x999;
            insertProcQ(&external_blocked_list[line - 3][dev], toBlock); // Metto line - 3 per mappare le costanti al vettore
            break;
          case IL_FLASH:
            debug = 0x998;
            insertProcQ(&external_blocked_list[line - 3][dev], toBlock); // Metto line - 3 per mappare le costanti al vettore
            break;
          case IL_ETHERNET:
            debug = 0x997;
            insertProcQ(&external_blocked_list[line - 3][dev], toBlock); // Metto line - 3 per mappare le costanti al vettore
            break;
          case IL_PRINTER:
            debug = 0x996;
            insertProcQ(&external_blocked_list[line - 3][dev], toBlock); // Metto line - 3 per mappare le costanti al vettore
            break;
        }
      }
    }
  }

  waiting_count++;
  *arg->commandAddr = arg->commandValue;
}