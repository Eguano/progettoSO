#include "./headers/pcb.h"

static pcb_t pcbTable[MAXPROC];
LIST_HEAD(pcbFree_h);
static int next_pid = 1;

/**
 * @brief Inserisce tutti i processi nella lista dei processi liberi. (chiamata solo all'inizio dell'eseczuione)
 * 
 */
void initPcbs() {
    for(int i = 0; i < MAXPROC; i++){
        list_add(&pcbTable[i].p_list, &pcbFree_h);
    }
}

/**
 * @brief Inserisce il processo puntato da p nella lista dei processi liberi.
 * 
 * @param p 
 */
void freePcb(pcb_t *p) {
    list_add_tail(&(p->p_list), &pcbFree_h);
}

/**
 * @brief Se sono disponibili processi liberi, inizializza i valori di uno e lo rimuove dalla lista dei processi liberi.
 * 
 * @return pcb_t* 
 */
pcb_t *allocPcb() {
    if(list_empty(&pcbFree_h))
        return NULL;
    else{
        pcb_PTR tempPcb = container_of(pcbFree_h.next, pcb_t, p_list);
        list_del(pcbFree_h.next);
        INIT_LIST_HEAD(&tempPcb->p_list);
        INIT_LIST_HEAD(&tempPcb->p_child);
        INIT_LIST_HEAD(&tempPcb->p_sib);
        INIT_LIST_HEAD(&tempPcb->msg_inbox);
        tempPcb->p_parent = NULL;
        tempPcb->p_time = 0;
        tempPcb->p_supportStruct = NULL;
        tempPcb->p_pid = next_pid;
        next_pid++;
        tempPcb->p_s.status = ALLOFF;
        return tempPcb;
    }
}

/**
 * @brief Inizializza la sentinella della process queue.
 * 
 * @param head sentinella della lista da inizializzare
 */
void mkEmptyProcQ(struct list_head *head) {
    INIT_LIST_HEAD(head);
}

/**
 * @brief Controlla se la process queue è vuota.
 * 
 * @param head sentinella della lista da controllare
 * @return 1 se è vuota, 0 altrimenti 
 */
int emptyProcQ(struct list_head *head) {
    return list_empty(head);
}

/**
 * @brief Inserisce un processo in coda alla process queue.
 * 
 * @param head sentinella della lista in cui inserire 
 * @param p processo da inserire
 */
void insertProcQ(struct list_head *head, pcb_t *p) {
    list_add_tail(&p->p_list, head);
}

/**
 * @brief Ritorna, senza rimuovere, il primo processo nella process queue. 
 * 
 * @param head sentinella della lista da cui leggere
 * @return pcb_t*
 */
pcb_t *headProcQ(struct list_head *head) {
    if(emptyProcQ(head))
        return NULL;
    else
        return container_of(head->next, pcb_t, p_list);
}

/**
 * @brief Rimuove e ritorna il primo processo nella process queue.
 * 
 * @param head sentinella della lista da cui leggere e rimuovere
 * @return pcb_t* 
 */
pcb_t *removeProcQ(struct list_head *head) {
    if(emptyProcQ(head))
        return NULL;
    else {
        pcb_PTR temp = headProcQ(head);
        list_del(&temp->p_list);
        return temp;
    }
}

/**
 * @brief Rimuove dalla process queue il processo puntato da p. 
 * 
 * @param head sentinella della lista da cui leggere e rimuovere
 * @param p processo da rimuovere
 * @return pcb_t* 
 */
pcb_t *outProcQ(struct list_head *head, pcb_t *p) {
    pcb_PTR temp;
    list_for_each_entry(temp, head, p_list) {
        if(temp == p) {
            list_del(&p->p_list);
            return p;
        }
    }
    return NULL;
}

/**
 * Controlla se il pcb è nella lista degli inutilizzati
 * 
 * @param p puntatore al pcb da cercare
 * @return 1 se è nella lista free, 0 altrimenti
 */
int isInPCBFree_h(pcb_t *p) {
    pcb_PTR iter;
    list_for_each_entry(iter, &pcbFree_h, p_list) {
        if(iter == p) {
            return 1;
        }
    }
    return 0;
}

/**
 * Controlla se un pcb fa parte di una lista
 * 
 * @param head sentinella della lista in cui controllare
 * @param p puntatore al pcb da cercare
 * @return 1 se è presente nella lista, 0 altrimenti
 */
int isInList(struct list_head *head, pcb_t *p) {
    pcb_PTR iter;
    list_for_each_entry(iter, head, p_list) {
        if(iter == p) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief True se il processo puntato da p non ha figli, false altrimenti.
 * 
 * @param p 
 * @return int 
 */
int emptyChild(pcb_t *p) {
    return list_empty(&p->p_child) ? 1 : 0;
}

/**
 * @brief Inserisce il processo puntato da p come figlio del processo puntato da prnt.
 * 
 * @param prnt 
 * @param p 
 */
void insertChild(pcb_t *prnt, pcb_t *p) {
    p->p_parent = prnt;
    list_add_tail(&p->p_sib, &prnt->p_child);
}

/**
 * @brief Rimuove il primo figlio del processo puntato da p. Ritorna NULL se il processo non ha figli.
 * 
 * @param p 
 * @return pcb_t* 
 */
pcb_t *removeChild(pcb_t *p) {
    if (!list_empty(&p->p_child)){
        // pcb_PTR tempPcb = container_of(p->p_child.next, pcb_t, p_list);
        pcb_PTR tempPcb = container_of(p->p_child.next, pcb_t, p_sib);
        tempPcb->p_parent = NULL;
        // list_del(p->p_child.next);
        list_del(&tempPcb->p_sib);
        return tempPcb;
    }
    else
        return NULL;
}

/**
 * @brief Rimuove il processo puntato da p dalla lista dei figli del processo padre. Ritorna NULL se il processo non ha un padre.
 * 
 * @param p 
 * @return pcb_t* 
 */
pcb_t *outChild(pcb_t *p) {
    if (p->p_parent != NULL){
        list_del(&p->p_sib);
        p->p_parent = NULL;
        return p;
    }
    else
        return NULL;
}