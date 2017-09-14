#ifndef _KERNEL__SYSCALL_HANDLERS_H_
#define _KERNEL__SYSCALL_HANDLERS_H_

#include "kernel/kernel_ds.h"
#include "kernel/scheduler.h"
#include "common/byte_buffer.h"

int syscall_send(
  struct TaskDescriptor tds[],
  struct TaskAllocationData *alloc_data,
  struct Scheduler *scheduler,
  struct ByteBuffer sendQueues[],
  struct MessageRequest messageRequests[],
  int tid, char *msg, int msglen, char *reply, int rplen
);

void syscall_receive(
  struct TaskDescriptor tds[],
  struct Scheduler *scheduler,
  struct ByteBuffer sendQueues[],
  struct MessageRequest messageRequests[],
  int *tid, char *msg, int msglen
);

int syscall_reply(
  struct TaskDescriptor tds[],
  struct TaskAllocationData *alloc_data,
  struct Scheduler *scheduler,
  struct ByteBuffer sendQueues[],
  struct MessageRequest messageRequests[],
  int tid, char *reply, int rplen
);

int syscall_create(
  struct TaskDescriptor tds[],
  struct TaskAllocationData *alloc_data,
  struct Scheduler *scheduler,
  int parent_tid,
  int priority,
  void (*entryPoint)()
);

int syscall_await_event(
  struct TaskDescriptor tds[],
  struct Scheduler *scheduler,
  struct AwaitedEvents *awaited_events,
  int eventid
);

#endif
