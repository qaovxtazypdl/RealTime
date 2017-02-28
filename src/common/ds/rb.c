#include <types.h>
#include <ds/rb.h>

/* TODO optimize */

/* Must be called before a ring buffer can be used, caller must ensure the memory persists throughout the life */
/* of the buffer and is responsible for freeing it. */

void rb_init(RB *buf, char *mem, int32_t sz) {
  buf->mem = mem;
  buf->sz = sz;
  buf->tail = 0;
  buf->head = 0;
  buf->full = 0;
};

/* Returns 0 on success, peak at the next elements of the queue without removing it */
int rb_peak(RB *buf, char *res) {
  if(buf->tail == buf->head && !buf->full)
    return 1;

  *res = buf->mem[buf->head];
  return 0;
}

/* Returns 0 on success */
int rb_shift(RB *buf, char *res) {
  if(buf->tail == buf->head && !buf->full)
    return 1;

  if(*res)
    *res = buf->mem[buf->head];
  buf->head = (buf->head + 1) % buf->sz;
  buf->full = 0;
  return 0;
}

/* Returns 0 on success (i.e buffer is not full) */
int rb_push(RB *buf, char byte) {
  if(buf->full)
    return 1;

  buf->mem[buf->tail] = byte;
  buf->tail = (buf->tail + 1) % buf->sz;

  if(buf->tail == buf->head)
    buf->full = 1;

  return 0;
}

/* Push a null terminated array of bytes (all or nothing), returns 1 on success. */

int rb_pushstr(RB *buf, const char *bytes) {
  int orig_end = buf->tail;
  
  while(*bytes && !buf->full) {
    buf->mem[buf->tail] = *bytes;
    buf->tail = (buf->tail + 1) % buf->sz;
    bytes++;
    if(buf->tail == buf->head)
      buf->full = 1;
  }
  
  if(buf->full && *bytes) {
  buf->full = 0;
  buf->tail = orig_end;
  return 1;
  }
  
  return 0;
}
