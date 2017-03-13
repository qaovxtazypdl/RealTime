#ifndef _DBG_H_
#define _DBG_H_
#include <common/bwio.h>

#define dbgt(arg0, ...) { \
    kern_print("DELETE ME: %s (l%d): "arg0"\n\r",__func__, __LINE__, ##__VA_ARGS__); \
}
#define dbgtc(cond, arg0, ...) { \
  if((cond)) { \
    bwprintf(COM2, "DELETE ME: %s: "arg0"\n\r",__func__, ##__VA_ARGS__); \
  } \
}
#endif
