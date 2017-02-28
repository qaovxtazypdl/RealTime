#ifndef _QUEUE_H_
#define _QUEUE_H_

typedef struct {
  int mem_sz;
  int memb_sz;
  void *mem;
  int tail;
  int head;
  char empty;
} queue_t;

void queue_init(queue_t *q, void *mem, int sz, int memb_sz);
int queue_add(queue_t *q, void *item);
int queue_consume(queue_t *q, void *item);
int queue_peek(queue_t *q, void *item);
int queue_add_multiple(queue_t *q, void *start, int nitems);

#endif
