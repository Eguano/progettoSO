#include "vmSupport.h"

extern pcb_PTR ssi_pcb;

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
        // TODO: Garantire la mutua esclusione sulla swap pool 
        // mandando un messaggio al processo mutex per la swap pool

        unsigned int missingPageNum = support_PTR->sup_exceptState[PGFAULTEXCEPT].entry_hi;

    }

}