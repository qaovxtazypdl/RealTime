/* Ring buffer implementation */

#ifndef _RB_H_
#define _RB_H_
#include<types.h>

/* TODO optimize */

typedef struct rb {
  char *mem;
  int32_t sz;
  int32_t head;
  int32_t tail;
  int32_t full;
} RB;

int rb_pushstr(RB *buf, const char *bytes);
int rb_push(RB *buf, char byte);
int rb_shift(RB *buf, char *res);
int rb_peak(RB *buf, char *res);
void rb_init(RB *buf, char *mem, int32_t sz);

#endif
