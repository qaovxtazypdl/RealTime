#ifndef _SCHED_H_
#define _SCHED_H_
#include <task.h>

void init_sched();
int sched_add_task(task_t *task);
task_t *sched_consume_task();
task_t *sched_next_task();

#define ERR_TASK_READY -1
#define ERR_QUEUE_FULL -2

#define clz __builtin_clz /* TODO find a sane place for this */
#endif
