#include <ds/queue.h>
#include <mem.h>

void inline queue_init(queue_t *q, void *mem, int sz, int memb_sz) {
  q->mem_sz = sz;
  q->memb_sz = memb_sz;
  q->mem = mem;
  q->tail = 0;
  q->head = 0;
  q->empty = 1;
}


int inline queue_add(queue_t *q, void *item) {
  if(q->tail == q->head && !q->empty)
    return 1;

  q->empty = 0;
  memcpy(q->tail + q->mem, item, q->memb_sz);
  q->tail = (q->tail + q->memb_sz) % q->mem_sz;

  return 0;
}
  
int inline queue_consume(queue_t *q, void *item) {
  if(q->empty)
    return 1;
  memcpy(item, q->mem + q->head, q->memb_sz);
  q->head = (q->head + q->memb_sz) % q->mem_sz;
  q->empty = q->head == q->tail;

  return 0;
}

int inline queue_peek(queue_t *q, void *item) {
  if(q->empty)
    return 1;

  memcpy(item, q->mem + q->head, q->memb_sz);
  return 0;
}

int inline queue_add_multiple(queue_t *q, void *start, int nitems) {

  /* FIXME */
  int i;
  for (i = 0; i < nitems; i++) {
    if(queue_add(q, start + (i * q->memb_sz)))
      return 1;
  }
  /* if((((q->tail - q->head) % q->mem_sz) / q->memb_sz) < nitems) */
  /* 	memcpy(q->tail + q->mem, start, nitems * q->memb_sz); */

  /* q->empty = 0; */
  return 0;
}

