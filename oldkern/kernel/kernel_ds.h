#ifndef _KERNEL__KERNEL_DS_H_
#define _KERNEL__KERNEL_DS_H_

#include "kernel/constants.h"

struct AwaitedEvents {
  int num_missed_interrupts;
  struct TaskDescriptor *priority_list_heads[NUM_IRQ_MAX];
  struct TaskDescriptor *priority_list_tails[NUM_IRQ_MAX];
};

struct Scheduler {
  // NUM_SCHEDULER_PRIORITIES priorities queue pointers
  unsigned int nonempty_queue_bitmask;
  int flag_shutdown;
  struct TaskDescriptor *active;
  char clz_lookup_map[32];
  struct TaskDescriptor *priority_list_heads[NUM_SCHEDULER_PRIORITIES];
  struct TaskDescriptor *priority_list_tails[NUM_SCHEDULER_PRIORITIES];
};

struct TaskDescriptor {
  /* 0x00 */ int tid;
  /* 0x04 */ int *sp;
  /* 0x08 */ int spsr;
  /* 0x0c */ int *pc;
  /* 0x10 */ int return_val;
  /* 0x18 */ int priority;
  /* 0x1c */ int parent_tid;
  /* 0x20 */ int status;
  /* 0x24 */ struct TaskDescriptor *next;
};

struct KernelRequest {
  /* 0x00 */ int request_id;
  /* 0x04 */ int arg0;
  /* 0x08 */ int arg1;
  /* 0x0c */ int arg2;
  /* 0x10 */ int arg3;
  /* 0x14 */ int *user_sp;
};

struct TaskAllocationData {
  int next_stack_pointer;
  int num_tasks_allocated;
};

struct TaskProfilerData {
  int profile_start_time;
  int kernel_time;
  int task_time[NUM_TASKS_MAX];
};

struct MessageRequest {
  int *tid;
  char *msg;
  int msglen;
  char *reply;
  int rplen;
};

#endif
