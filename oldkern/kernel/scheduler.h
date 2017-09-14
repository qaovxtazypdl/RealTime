#ifndef _KERNEL__SCHEDULER_H_
#define _KERNEL__SCHEDULER_H_

#include "kernel/constants.h"
#include "kernel/kernel_ds.h"

void scheduler_init(struct Scheduler *scheduler);
void scheduler_add_td(struct Scheduler *scheduler, struct TaskDescriptor *td);
void scheduler_remove_active_from_queue(struct Scheduler *scheduler);
void scheduler_move_active_to_ready(struct Scheduler *scheduler);

#endif
