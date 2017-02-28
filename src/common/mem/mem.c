#include <mem.h>

void memcpy(void *dst, const void *src, int nbytes) {
  char *d = dst;
  const char *s = src;

  while(nbytes--)
    *d++ = *s++;
}
