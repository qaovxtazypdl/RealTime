#ifndef _ASSERT_H_
#define _ASSERT_H_
#include <common/syscall.h>
#include <common/bwio.h>

#define assert(cond, fmt, ...) { \
if(!(cond)) {                                                       \
    bwprintf(COM2, "\r\n=================================================================================\r\n"); \
    bwprintf(COM2, "ASSERTION FAILURE: in %s (l%d): " fmt "\r\n", __func__, __LINE__, ##__VA_ARGS__); \
    bwprintf(COM2, "=================================================================================\r\n"); \
    SYSCALL(SYSCALL_TERMINATE);                                         \
  } \
}

#endif
