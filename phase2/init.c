#include "init.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"
#include "scheduler.h"

extern void uTLB_RefillHandler();
extern void exceptionHandler();
extern void SSIHandler();
extern void test();

/**
 * Entry point del sistema operativo.
 * <p>Inizializza il nucleo, istanzia i processi SSI e test, carica l'Interval Timer
 */
void main() {
  // nucleus initialization
  initialize();

  // load Interval Timer 100ms
  LDIT(PSECOND);

  // istantiate a first process
  ssi_pcb = allocPcb();
  ssi_pcb->p_s.status |= IEPON | IMON;
  RAMTOP(ssi_pcb->p_s.reg_sp);
  ssi_pcb->p_s.pc_epc = (memaddr) SSIHandler;
  ssi_pcb->p_s.reg_t9 = (memaddr) SSIHandler;
  insertProcQ(&ready_queue, ssi_pcb);
  process_count++;

  // istantiate a second process
  p3test_pcb = allocPcb();
  p3test_pcb->p_s.status |= IEPON | IMON | TEBITON;
  p3test_pcb->p_s.reg_sp = ssi_pcb->p_s.reg_sp - (2 * PAGESIZE);
  p3test_pcb->p_s.pc_epc = p3test_pcb->p_s.reg_t9 = (memaddr) test;
  insertProcQ(&ready_queue, p3test_pcb);
  process_count++;

  // call the scheduler
  schedule();
}

/**
 * Inizializza il PassUp Vector, le strutture di pcb/msg e
 * le variabili globali
 */
static void initialize() {
  // Pass Up Vector for Processor 0
  passupvector_t *passUpVec = (passupvector_t *) PASSUPVECTOR;
  passUpVec->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
  passUpVec->tlb_refill_stackPtr = (memaddr) KERNELSTACK;
  passUpVec->exception_handler = (memaddr) exceptionHandler;
  passUpVec->exception_stackPtr = (memaddr) KERNELSTACK;

  // level 2 structures
  initPcbs();
  initMsgs();

  // initialize variables
  process_count = 0;
  waiting_count = 0;
  current_process = NULL;
  mkEmptyProcQ(&ready_queue);
  for (int i = 0; i < MAXDEV; i++) {
    // external
    mkEmptyProcQ(&external_blocked_list[0][i]);
    mkEmptyProcQ(&external_blocked_list[1][i]);
    mkEmptyProcQ(&external_blocked_list[2][i]);
    mkEmptyProcQ(&external_blocked_list[3][i]);
    // terminal
    mkEmptyProcQ(&terminal_blocked_list[0][i]);
    mkEmptyProcQ(&terminal_blocked_list[1][i]);
  }
  mkEmptyProcQ(&pseudoclock_blocked_list);
  currentState = (state_t *)BIOSDATAPAGE;
  stateCauseReg = &currentState->cause;
}


/**
 * Controlla se il processo è in attesa di un device
 * 
 * @param p puntatore al processo da cercare
 * @return 1 se è in una lista di device, 0 altrimenti
 */
int isInDevicesLists(pcb_t *p) {
  for (int i = 0; i < MAXDEV; i++) {
    if (isInList(&external_blocked_list[0][i], p)) return TRUE;
    if (isInList(&external_blocked_list[1][i], p)) return TRUE;
    if (isInList(&external_blocked_list[2][i], p)) return TRUE;
    if (isInList(&external_blocked_list[3][i], p)) return TRUE;
    if (isInList(&terminal_blocked_list[0][i], p)) return TRUE;
    if (isInList(&terminal_blocked_list[1][i], p)) return TRUE;
  }
  return FALSE;
}

/**
 * Copia tutti i registri dello state
 * 
 * @param dest stato in cui copiare i valori
 * @param src stato da cui copiare i valori
 */
void copyRegisters(state_t *dest, state_t *src) {
  dest->cause = src->cause;
  dest->entry_hi = src->entry_hi;
  dest->reg_at = src->reg_at;
  dest->reg_v0 = src->reg_v0;
  dest->reg_v1 = src->reg_v1;
  dest->reg_a0 = src->reg_a0;
  dest->reg_a1 = src->reg_a1;
  dest->reg_a2 = src->reg_a2;
  dest->reg_a3 = src->reg_a3;
  dest->reg_t0 = src->reg_t0;
  dest->reg_t1 = src->reg_t1;
  dest->reg_t2 = src->reg_t2;
  dest->reg_t3 = src->reg_t3;
  dest->reg_t4 = src->reg_t4;
  dest->reg_t5 = src->reg_t5;
  dest->reg_t6 = src->reg_t6;
  dest->reg_t7 = src->reg_t7;
  dest->reg_s0 = src->reg_s0;
  dest->reg_s1 = src->reg_s1;
  dest->reg_s2 = src->reg_s2;
  dest->reg_s3 = src->reg_s3;
  dest->reg_s4 = src->reg_s4;
  dest->reg_s5 = src->reg_s5;
  dest->reg_s6 = src->reg_s6;
  dest->reg_s7 = src->reg_s7;
  dest->reg_t8 = src->reg_t8;
  dest->reg_t9 = src->reg_t9;
  dest->reg_gp = src->reg_gp;
  dest->reg_sp = src->reg_sp;
  dest->reg_fp = src->reg_fp;
  dest->reg_ra = src->reg_ra;
  dest->reg_HI = src->reg_HI;
  dest->reg_LO = src->reg_LO;
  dest->hi = src->hi;
  dest->lo = src->lo;
  dest->pc_epc = src->pc_epc;
  dest->status = src->status;
}