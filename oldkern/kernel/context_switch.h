#ifndef _KERNEL__CONTEXT_SWITCH_H_
#define _KERNEL__CONTEXT_SWITCH_H_

#include <ts7200.h>

struct TaskDescriptor;
struct KernelRequest;

void kernel_entry_irq();
void kernel_entry();
void kernel_exit(struct TaskDescriptor *active, struct KernelRequest *req);

#endif
