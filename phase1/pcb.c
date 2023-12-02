#include "./headers/pcb.h"

static pcb_t pcbTable[MAXPROC];
LIST_HEAD(pcbFree_h);
static int next_pid = 1;

void initPcbs() {
    for(int i = 0; i < MAXPROC; i++){
        list_add(&pcbTable[i].p_list, &pcbFree_h);
    }
}

void freePcb(pcb_t *p) {
    list_add_tail(&(p->p_list), &pcbFree_h);
}

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
        return tempPcb;
    }
}

void mkEmptyProcQ(struct list_head *head) {
    INIT_LIST_HEAD(head);
}

int emptyProcQ(struct list_head *head) {
    return list_empty(head);
}

void insertProcQ(struct list_head *head, pcb_t *p) {
    list_add_tail(&p->p_list, head);
}

pcb_t *headProcQ(struct list_head *head) {
    if(emptyProcQ(head))
        return NULL;
    else
        return container_of(head->next, pcb_t, p_list);
}

pcb_t *removeProcQ(struct list_head *head) {
    if(emptyProcQ(head))
        return NULL;
    else {
        pcb_PTR temp = headProcQ(head);
        list_del(&temp->p_list);
        return temp;
    }
}

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

int emptyChild(pcb_t *p) {
    return list_empty(&p->p_child) ? 1 : 0;
}

void insertChild(pcb_t *prnt, pcb_t *p) {
    p->p_parent = prnt;
    list_add(&p->p_sib, &prnt->p_child);
}

pcb_t *removeChild(pcb_t *p) {
    if (!list_empty(&p->p_child)){
        pcb_PTR tempPcb = container_of(p->p_child.next, pcb_t, p_list);
        list_del(p->p_child.next);
        return tempPcb;
    }
    else
        return NULL;
}

pcb_t *outChild(pcb_t *p) {
    if (p->p_parent != NULL){
        list_del(&p->p_sib);
        p->p_parent = NULL;
    }
}
