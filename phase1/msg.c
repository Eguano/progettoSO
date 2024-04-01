#include "./headers/msg.h"

static msg_t msgTable[MAXMESSAGES];
LIST_HEAD(msgFree_h);

/**
 * @brief Inizializza la lista dei messaggi inutilizzati.
 * 
 */
void initMsgs() {
  for (int i = 0; i < MAXMESSAGES; i++) {
    list_add(&msgTable[i].m_list, &msgFree_h);
  }
}

/**
 * @brief Libera un messaggio e lo reinserisce nella lista dei messaggi inutilizzati.
 * 
 * @param m puntatore al messaggio da liberare
 */
void freeMsg(msg_t *m) {
  list_add_tail(&(m->m_list), &msgFree_h);
}

/**
 * @brief Alloca un messaggio vuoto.
 * 
 * @return puntatore al messaggio se allocato, NULL altrimenti
 */
msg_t *allocMsg() {
  if (list_empty(&msgFree_h)) {
    return NULL;
  } else {
    msg_PTR m = container_of(msgFree_h.next, msg_t, m_list);
    list_del(msgFree_h.next);
    INIT_LIST_HEAD(&m->m_list);
    m->m_sender = NULL;
    m->m_payload = 0;
    return m;
  }
}

/**
 * @brief Inizializza la sentinella per una message queue.
 * 
 * @param head sentinella della lista da inizializzare
 */
void mkEmptyMessageQ(struct list_head *head) {
  INIT_LIST_HEAD(head);
}

/**
 * @brief Controlla se la message queue è vuota.
 * 
 * @param head sentinella della lista da controllare
 * @return 1 se è vuota, 0 altrimenti
 */
int emptyMessageQ(struct list_head *head) {
  return list_empty(head);
}

/**
 * @brief Inserisce un messaggio in coda alla message queue.
 * 
 * @param head sentinella della lista in cui inserire
 * @param m messaggio da inserire
 */
void insertMessage(struct list_head *head, msg_t *m) {
  list_add_tail(&m->m_list, head);
}

/**
 * @brief Inserisce un messaggio in testa alla message queue.
 * 
 * @param head sentinella della lista in cui inserire
 * @param m messaggio da inserire
 */
void pushMessage(struct list_head *head, msg_t *m) {
  list_add(&m->m_list, head);
}

/**
 * @brief Rimuove il primo messaggio nella message queue inviato da p_ptr.
 * Se p_ptr è NULL rimuove il messaggio in testa.
 * 
 * @param head sentinella della lista da cui rimuovere
 * @param p_ptr puntatore al PCB mittente richiesto
 * @return puntatore al messaggio dal mittente richiesto se trovato, NULL altrimenti
 */
msg_t *popMessage(struct list_head *head, pcb_t *p_ptr) {
  if (list_empty(head)) {
    return NULL;
  } else {
    if (p_ptr == NULL) {
      msg_PTR m = container_of(list_next(head), msg_t, m_list);
      list_del(list_next(head));
      return m;
    } else {
      msg_PTR i = NULL;
      list_for_each_entry(i, head, m_list) {
        if (i->m_sender == p_ptr) {
          list_del(&i->m_list);
          return i;
        }
      }
      return NULL;
    }
  }
}

/**
 * @brief Legge il messaggio in testa alla message queue.
 * 
 * @param head sentinella della lista da cui leggere
 * @return puntatore al messaggio in testa
 */
msg_t *headMessage(struct list_head *head) {
  if (list_empty(head)) {
    return NULL;
  } else {
    return container_of(list_next(head), msg_t, m_list);
  }
}
