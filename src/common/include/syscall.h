#ifndef _SYSCALL_H_
#define _SYSCALL_H_
#include <common/event.h>

#define SYS_STORE_RESULT(var) \
  asm("mov %0, r0\r\t" : "=r" (var));

/* sadly I must credit S.O for this piece of macro brilliance. */
#define WHICH_SYS_POPULATE_ARGS(_v1, _v2, _v3, _v4, _v5, NAME, ...) NAME
#define SYS_POPULATE_ARGS(...) WHICH_SYS_POPULATE_ARGS(__VA_ARGS__, SYS_POPULATE_ARGS_5, SYS_POPULATE_ARGS_4, SYS_POPULATE_ARGS_3, SYS_POPULATE_ARGS_2, SYS_POPULATE_ARGS_1)(__VA_ARGS__)

#define SYS_POPULATE_ARGS_1(v1) { \
  asm volatile ("mov r0, %0" : : "r" (v1): "r3", "r2", "r1", "r0"); \
}

#define SYS_POPULATE_ARGS_2(v1, v2) { \
  asm volatile ("mov r0, %0" : : "r" (v1): "r3", "r2", "r1", "r0"); \
  asm volatile ("mov r1, %0" : : "r" (v2) : "r3", "r2", "r1", "r0"); \
}

#define SYS_POPULATE_ARGS_3(v1, v2, v3) { \
  asm volatile ("mov r0, %0" : : "r" (v1): "r3", "r2", "r1", "r0"); \
  asm volatile ("mov r1, %0" : : "r" (v2) : "r3", "r2", "r1", "r0"); \
  asm volatile ("mov r2, %0" : : "r" (v3) : "r3", "r2", "r1", "r0"); \
}

#define SYS_POPULATE_ARGS_4(v1, v2, v3, v4) { \
  asm volatile ("mov r0, %0" : : "r" (v1): "r3", "r2", "r1", "r0"); \
  asm volatile ("mov r1, %0" : : "r" (v2) : "r3", "r2", "r1", "r0"); \
  asm volatile ("mov r2, %0" : : "r" (v3) : "r3", "r2", "r1", "r0"); \
  asm volatile ("mov r3, %0" : : "r" (v4) : "r3", "r2", "r1", "r0"); \
}

#define SYS_POPULATE_ARGS_5(v1, v2, v3, v4, v5) { \
  asm volatile ("mov r0, %0" : : "r" (v1): "r3", "r2", "r1", "r0"); \
  asm volatile ("mov r1, %0" : : "r" (v2) : "r3", "r2", "r1", "r0"); \
  asm volatile ("mov r2, %0" : : "r" (v3) : "r3", "r2", "r1", "r0"); \
  asm volatile ("mov r3, %0" : : "r" (v4) : "r3", "r2", "r1", "r0"); \
  asm volatile ("mov ip, %0" : : "r" (v5) : "r3", "r2", "r1", "r0"); \
}

/* An enum is avoided to allow our SYSCALL macro magic to work, a function could be used */
/* instead but meh :/ */

#define SYSCALL_INITIALIZATION 0
#define SYSCALL_EXIT 1
#define SYSCALL_CREATE 2
#define SYSCALL_MYTID 3
#define SYSCALL_PARENT_TID 4
#define SYSCALL_PASS 5
#define SYSCALL_SEND 6
#define SYSCALL_RECEIVE 7
#define SYSCALL_REPLY 8
#define SYSCALL_AWAIT_EVENT 9
#define SYSCALL_TERMINATE 10

static char *syscall2str[] = {
  "SYSCALL_INITIALIZATION",
  "SYSCALL_EXIT",
  "SYSCALL_CREATE",
  "SYSCALL_MYTID",
  "SYSCALL_PARENT_TID",
  "SYSCALL_PASS",
  "SYSCALL_SEND",
  "SYSCALL_RECEIVE",
  "SYSCALL_REPLY",
  "SYSCALL_AWAIT_EVENT",
  "SYSCALL_TERMINATE"
};

#define str(a) #a
#define SYSCALL(syscall) asm volatile("swi "str(syscall)) /* Used by the userspace library to trap into the kernel */

/* Userspace functions which set up arguments and trap into the kernel via SWI */

int create(int priority, void (*start)());
int my_tid();
int my_parent_tid();
void pass();
void exit();
int await_event(enum event type, void *payload);
int send(int tid, const void *msg, int msg_sz, void *reply, int repl_sz);
int receive(int *td, void *buf, int buf_len);
int reply(int td, void *msg, int msg_len);

#endif
