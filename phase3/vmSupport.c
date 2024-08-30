#include "vmSupport.h"

extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern pcb_PTR mutexHolderProcess;
extern pcb_PTR swapMutexProcess;
extern swpo_t *swap_pool;


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
        supportTrapHandler(&support_PTR->sup_exceptState[PGFAULTEXCEPT]);
    }
    else {
        // Garantisco la mutua esclusione sulla swap pool 
        // mandando un messaggio al processo mutex per la swap pool
        if (current_process != mutexHolderProcess) {
            SYSCALL(SENDMESSAGE, (unsigned int)swapMutexProcess, 0, 0);
            SYSCALL(RECEIVEMESSAGE, (unsigned int)swapMutexProcess, 0, 0);
        }

        unsigned int p = (support_PTR->sup_exceptState[PGFAULTEXCEPT].entry_hi >> 12) & VPNMASK;

        // Scelgo un frame da sostituire
        unsigned int i = selectFrame();

        if (swap_pool[i]->swpo_asid != NOPROC) {
            invalidateFrame(i, support_PTR);
        }

        // Leggo la pagina dal backing store al frame
        memaddr frameAddr = (memaddr) FRAMEPOOLSTART + (i * PAGESIZE);
        dtpreg_t *flashDevReg = (dtpreg_t *)GET_DEV_REG(FLASHINT, support_PTR->sup_asid);
        readWriteBackingStore(flashDevReg, frameAddr, p, FLASHREAD);

        // Aggiorno la swap pool entry
        swap_pool[i]->swpo_asid = support_PTR->sup_asid;
        swap_pool[i]->swpo_page = p;
        // ASSICURARSI CHE LA USER PAGE TABLE SIA INIZIALIZZATA PER TUTTI I PROCESSI
        swap_pool[i]->swpo_pte_ptr = &support_PTR->sup_privatePgTbl[p];

        // Disabilito gli interrupt
        setSTATUS(getSTATUS() & (~IECON));

        // Aggiorno Valid e Dirty nella page table entry
        support_PTR->sup_privatePgTbl[p].pte_entryLO |= VALIDON;
        support_PTR->sup_privatePgTbl[p].pte_entryLO &= (~DIRTYON);
        // Metto a 0 i bit per il PFN e lo aggiorno
        support_PTR->sup_privatePgTbl[p].pte_entryLO &= 0xFFF;
        support_PTR->sup_privatePgTbl[p].pte_entryLO |= (i << 12);

        // Aggiorno il TLB
        updateTLB(&support_PTR->sup_privatePgTbl[p]);

        // Riabilito gli interrupt
        setSTATUS(getSTATUS() | IECON);

        // Rilascio la mutex
        SYSCALL(SENDMESSAGE, (unsigned int)swapMutexProcess, 0, 0);

        // Restituisco il controllo al current process
        LDST(&support_PTR->sup_exceptState[PGFAULTEXCEPT]);
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

void invalidateFrame(unsigned int frame, support_t *support_PTR){
    // Disabilito gli interrupt
    setSTATUS(getSTATUS() & (~IECON));

    // Nego valid
    swap_pool[frame]->swpo_pte_ptr->pte_entryLO &= (~VALIDON);

    // Aggiorno il TLB
    updateTLB(swap_pool[frame]->swpo_pte_ptr);
    
    // Riabilito gli interrupt
    setSTATUS(getSTATUS() | IECON);

    // Aggiorno il backing store
    memaddr frameAddr = (memaddr) FRAMEPOOLSTART + (frame * PAGESIZE);
    dtpreg_t *flashDevReg = (dtpreg_t *)GET_DEV_REG(FLASHINT, swap_pool[frame]->swpo_asid);
    unsigned int res = readWriteBackingStore(flashDevReg, frameAddr, swap_pool[frame]->swpo_page, FLASHWRITE);

    if (res != 1) {
        supportTrapHandler(&support_PTR->sup_exceptState[PGFAULTEXCEPT]);
    }
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

/**
 * @brief Funzione incaricata per la lettura e scrittura all'interno dei backing store.
 * 
 * @param flashDevReg   Il device register del flash device coinvolto nell'operazione.
 * @param dataMemAddr   L'indirizzo di memoria iniziale del blocco RAM coinvolto nell'operazione.
 * @param devBlockNo    Determina quale blocco del flash device sara' coinvolto nell'operazione.
 * @param opType        FLASHREAD: il blocco devBlockNo del device flashDevReg verra' letto dentro a dataMemAddr
 *                      FLASHWRITE il valore di dataMemAddr verra' scritto nel blocco devBlockNo del device flashDevReg
 * 
 * @return              Il risultato dell'operazione di R/W
 */
unsigned int readWriteBackingStore(dtpreg_t *flashDevReg, memaddr dataMemAddr, unsigned int devBlockNo, unsigned int opType) {
    // Ricavo l'indirizzo del device register
    flashDevReg->data0 = dataMemAddr;

    // Preparo le strutture per l'SSI
    ssi_do_io_t doIO = {
        .commandAddr = (memaddr*)&flashDevReg->command,
        .commandValue = opType | (devBlockNo << 8)
    };

    ssi_payload_t payload = {
        .service_code = DOIO,
        .arg = &doIO
    };

    // Invio la richiesta
    unsigned int res;
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&res, 0);

    return res;
}