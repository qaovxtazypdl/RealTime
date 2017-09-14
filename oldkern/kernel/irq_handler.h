#ifndef _KERNEL__IRQ_HANDLER_H_
#define _KERNEL__IRQ_HANDLER_H_

#include "kernel/kernel_ds.h"

#define KERNEL_IRQ_ID 0x42

void handle_irq(
  struct TaskDescriptor tds[],
  struct Scheduler *scheduler,
  struct AwaitedEvents *awaited_events,
  int irq_1_status,
  int irq_2_status
);
#endif
