#include "ssi.h"

#include "../phase1/headers/pcb.h"

extern int process_count;
extern int waiting_count;
extern pcb_PTR current_process;
extern struct list_head ready_queue;
extern struct list_head external_blocked_list[4][MAXDEV];
extern struct list_head pseudoclock_blocked_list;
extern struct list_head terminal_blocked_list[2][MAXDEV];
extern pcb_PTR ssi_pcb;
extern pcb_PTR p2test_pcb;

extern int debug;
extern void klog_print(char *str);

/**
 * Gestisce una richiesta ricevuta da un processo
 */
void SSIHandler() {
  debug = 504;
  while (TRUE) {
    debug = 505;
    ssi_payload_PTR p_payload;
    pcb_PTR sender = (pcb_PTR) SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int) &p_payload, 0);
    debug = 506;
    // risposta da inviare al sender
    unsigned int response = (unsigned int) NULL;

    // esecuzione del servizio richiesto
    switch (p_payload->service_code) {
      case CREATEPROCESS:
        // creare un nuovo processo
        klog_print("VAMOOOSSS");
        debug = 507;
        response = createProcess((ssi_create_process_PTR) p_payload->arg, sender);
        debug = 508;
        break;
      case TERMPROCESS:
        // eliminare un processo esistente
        debug = 509;
        if (p_payload->arg == NULL) {
          debug = 510;
          terminateProcess(sender);
        } else {
          debug = 511;
          terminateProcess(p_payload->arg);
        }
        debug = 512;
        break;
      case DOIO:
        // eseguire un input o output
        // TODO: togliere processo che ha fatto richiesta dalla lista in cui si trova
        debug = 513;
        int dev = findDevice(((ssi_do_io_PTR) p_payload->arg)->commandAddr) / 10;
        debug = 514;
        int devInstance = findDevice(((ssi_do_io_PTR) p_payload->arg)->commandAddr) % 10;
        debug = 515;
        if (dev == 4) {
          debug = 516;
          // dispositivo terminale receiver
          insertProcQ(&terminal_blocked_list[1][devInstance], sender);
        } else if (dev == 5) {
          debug = 517;
          // dispositivo terminale transmitter
          insertProcQ(&terminal_blocked_list[0][devInstance], sender);
        } else {
          debug = 518;
          // dispositivo periferico
          insertProcQ(&external_blocked_list[dev][devInstance], sender);
        }
        waiting_count++;
        debug = 519;
        *((ssi_do_io_PTR) p_payload->arg)->commandAddr = ((ssi_do_io_PTR) p_payload->arg)->commandValue;
        debug = 520;
        // TODO: interruptHandler deve mandare un messaggio con il risultato dell'operazione
        break;
      case GETTIME:
        // restituire accumulated processor time
        debug = 521;
        response = (unsigned int) sender->p_time + (TIMESLICE - getTIMER());
        debug = 522;
        break;
      case CLOCKWAIT:
        // bloccare il processo per lo pseudoclock
        debug = 523;
        insertProcQ(&pseudoclock_blocked_list, sender);
        waiting_count++;
        debug = 524;
        break;
      case GETSUPPORTPTR:
        // restituire la struttura di supporto
        debug = 525;
        response = (unsigned int) sender->p_supportStruct;
        debug = 526;
        break;
      case GETPROCESSID:
        // restituire il pid del sender o del suo genitore
        debug = 527;
        if (p_payload->arg == 0) {
          debug = 528;
          response = sender->p_pid;
        } else {
          debug = 529;
          if (sender->p_parent == NULL) {
            debug = 530;
            response = 0;
          } else {
            debug = 531;
            response = sender->p_parent->p_pid;
          }
        }
        debug = 532;
        break;
      case ENDIO:
        // terminazione operazione IO
        debug = 533;
        sender = ((ssi_end_io_PTR) p_payload->arg)->toUnblock;
        response = ((ssi_end_io_PTR) p_payload->arg)->status;
        debug = 534;
        break;
      default:
        // codice non esiste, terminare processo richiedente e tutta la sua progenie
        debug = 535;
        terminateProcess(sender);
        debug = 536;
        break;
    }
    debug = 537;
    SYSCALL(SENDMESSAGE, (unsigned int) sender, response, 0);
    debug = 538;
  }
}

/**
 * Crea un processo come figlio del richiedente e lo posiziona in readyQueue
 * 
 * @param arg struttura contenente state e support (eventuale)
 * @param sender processo richiedente
 * @return puntatore al processo creato, NOPROC altrimenti
 */
static unsigned int createProcess(ssi_create_process_t *arg, pcb_t *sender) {
  pcb_PTR p = allocPcb();
  if (p == NULL) {
    return (unsigned int) NOPROC;
  } else {
    p->p_s = *arg->state;
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
    if (outProcQ(&ready_queue, p) == NULL) {
      // se non è nella ready lo cerco tra i bloccati per lo pseudoclock
      if (outProcQ(&pseudoclock_blocked_list, p) == NULL) {
        // se non è per lo pseudoclock, è bloccato per un altro interrupt
        int found = 0;
        for (int i = 0; i < MAXDEV && found == 0; i++) {
          if (outProcQ(&external_blocked_list[0][i], p) != NULL) found = 1;
          if (outProcQ(&external_blocked_list[1][i], p) != NULL) found = 1;
          if (outProcQ(&external_blocked_list[2][i], p) != NULL) found = 1;
          if (outProcQ(&external_blocked_list[3][i], p) != NULL) found = 1;
          if (outProcQ(&terminal_blocked_list[0][i], p) != NULL) found = 1;
          if (outProcQ(&terminal_blocked_list[1][i], p) != NULL) found = 1;
        }
      }
      // TODO: se viene terminato un processo in attesa per una receive ma non per un dispositivo, waiting_count non va decrementato: gestire in caso di problemi nel test
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