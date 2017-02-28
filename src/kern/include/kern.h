#ifndef _KERN_H_
#define _KERN_H_

#include <proc_state.h>
#include <task.h>

/* sadly I must credit S.O for this piece of macro brilliance. */
#define WHICH_RETURN(_v1, _v2, _v3, _v4, _v5, _v6, NAME, ...) NAME
#define RETURN(...) WHICH_RETURN(__VA_ARGS__, RETURN_5, RETURN_4, RETURN_3, RETURN_2, RETURN_1)(__VA_ARGS__)

#define RETURN_1(task, v1) \
  task->cpu_state->r0 = (int)v1; \
  sched_add_task(task);

#define RETURN_2(task, v1, v2) \
  task->cpu_state->r0 = (int)v1; \
  task->cpu_state->r1 = (int)v2; \
  sched_add_task(task);

#define RETURN_3(task, v1, v2, v3) \
  task->cpu_state->r0 = (int)v1; \
  task->cpu_state->r1 = (int)v2; \
  task->cpu_state->r2 = (int)v3; \
  sched_add_task(task);

#define RETURN_4(task, v1, v2, v3, v4) \
  task->cpu_state->r0 = (int)v1; \
  task->cpu_state->r1 = (int)v2; \
  task->cpu_state->r2 = (int)v3; \
  task->cpu_state->r3 = (int)v4; \
  sched_add_task(task);

#define RETURN_5(task, v1, v2, v3, v4, v5) \
  task->cpu_state->r0 = (int)v1; \
  task->cpu_state->r1 = (int)v2; \
  task->cpu_state->r2 = (int)v3; \
  task->cpu_state->r3 = (int)v4; \
  task->cpu_state->ip = (int)v5; \
  sched_add_task(task);

extern int kern_cts1;
extern int kern_cts2;
extern task_t *kern_current_task;

#endif
