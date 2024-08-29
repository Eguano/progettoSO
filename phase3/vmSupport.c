#include "vmSupport.h"

extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern pcb_PTR mutexHolderProcess;
extern pcb_PTR swapMutexProcess;


void TLB_ExceptionHandler() {

    // Ritiro la struttura di supporto di current process dalla SSI
    ssi_payload_t payload = {
        .service_code = GETSUPPORTPTR,
        .arg = NULL,
    };

    support_t *support_PTR;

    SYSCALL(SENDMESSAGE, (unsigned int) ssi_pcb, (unsigned int)(&payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int)(&support_PTR), 0);

    unsigned int exceptCause = support_PTR->sup_exceptState[PGFAULTEXCEPT].cause;

    // Se la causa e' di tipo TLB-Modification (Cause.ExecCode == 1) la gestisco come una trap
    if ((exceptCause & GETEXECCODE) >> CAUSESHIFT == 1) {
        supportTrapHandler(support_PTR->sup_exceptState[PGFAULTEXCEPT]);
    }
    else {
        // Garantisco la mutua esclusione sulla swap pool 
        // mandando un messaggio al processo mutex per la swap pool
        if (current_process != mutexHolderProcess) {
            SYSCALL(SENDMESSAGE, (unsigned int)swapMutexProcess, 0, 0);
            SYSCALL(RECEIVEMESSAGE, (unsigned int)swapMutexProcess, 0, 0);
        }

        unsigned int missingPageNum = (support_PTR->sup_exceptState[PGFAULTEXCEPT].entry_hi >> 12) & VPNMASK;

        // Scelgo un frame da sostituire
        unsigned int frameToReplace = selectFrame();

        if (swap_pool[frameToReplace]->swpo_asid != NOPROC) {
            invalidateFrame(frameToReplace);
        }

        // Rilascio la mutex

    }
}

static unsigned int selectFrame() {
    static int last = -1;

    for (int i = 0; i < POOLSIZE; i++) {
        if (swap_pool[i]->swpo_asid == NOPROC)
            return i;
    }

    return last = (last + 1) % POOLSIZE;
}

void invalidateFrame(unsigned int frame){
    // Disabilito gli interrupt
    setSTATUS(getSTATUS() & (~IECON));

    // Nego valid
    swap_pool[frame]->swpo_pte_ptr->pte_entryLO &= (~VALIDON);

    updateTLB(swap_pool[frame]->swpo_pte_ptr);

    // Riprendere da QUA

    // Riabilito gli interrupt
    setSTATUS(getSTATUS() & IECON);

}

void updateTLB(pteEntry_t *entry) {

    // Salvo il vecchio stato di entryHi e entryLo
    unsigned int oldEntryHi = getENTRYHI();
    unsigned int oldEntryLo = getENTRYLO();
    
    // Setto entryHi all'entryHi dell'entry da rimuovere
    setENTRYHI(entry->pte_entryHI);

    // Inizio un probe con TLBP(). Se viene trovata una entry corrispondente
    // all'interno del TLB, mi verra' segnalato tramite l'index
    TLBP();

    // Se lo trovo, setto anche entryLo cosi' da poter aggiornare la entry con TLBWI()
    if ((getINDEX() & PRESENTFLAG) == 0) {
        setENTRYLO(entry->pte_entryLO);
        // Aggiorna sia il campo entryHi che entryLo della entry del TLB attualmente puntata dall'index
        // (cioe' quella che vogliamo aggiornare)
        TLBWI();

        // Resetto entryLo
        setENTRYLO(oldEntryLo);
    }

    // Resetto entryHi
    setENTRYHI(oldEntryHi);

}