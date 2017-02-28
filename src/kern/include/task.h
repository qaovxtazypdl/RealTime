#ifndef _TASK_H_
#define _TASK_H_

#include <proc_state.h>

/* Convention: task descriptors start from 0 and are recycled (i.e they never exceed MAX_TASK) */
#define MAX_TASKS 200

typedef enum task_state {
  TASK_STATE_EVENT_BLOCKED,
  TASK_STATE_ACTIVE, /* Running */
  TASK_STATE_READY, /* Scheduled */
  TASK_STATE_NEW,  /* Newly created but unscheduled */
  TASK_STATE_BLOCKED,
  TASK_STATE_TERMINATED,
  TASK_STATE_RECEIVE_BLOCKED,
  TASK_STATE_SEND_BLOCKED,
  TASK_STATE_GET_BLOCKED
} task_state_t;

typedef struct task {
  int td;
  /* Parent td, -1 indicates that the parent is the kernel, this is only true for the first task. */
  int ptd; 
  unsigned int priority;
  enum task_state state;
  proc_state_t *cpu_state;
} task_t;

task_t *task_create(void (*start)(), unsigned int priority, int parent);
int task_destroy(task_t *task);
extern task_t *task_set[];

#endif

