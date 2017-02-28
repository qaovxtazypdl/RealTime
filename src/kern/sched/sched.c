#include <common/ds/queue.h>
#include <task.h>
#include <sched.h>

#define NUM_TASK_QUEUES (sizeof(int) * 8)
#define TASK_QUEUE_SIZE 10

static queue_t task_queues[NUM_TASK_QUEUES]; /* Task queues indexed by priority  */
static unsigned int active_queues = 0; /* Bit mask of active queues */
static int init = 0;

void init_sched() {
  int i;
  static struct task* task_queue_mem[NUM_TASK_QUEUES][TASK_QUEUE_SIZE];

  init++;

  for (i = 0; i < NUM_TASK_QUEUES; i++)
    queue_init(&task_queues[i],
               task_queue_mem[i],
               sizeof(task_queue_mem[0]),
               sizeof(task_queue_mem[0][0]));
}

int sched_add_task(struct task *task) {
  kassert(init == 1, "init_sched must be called exactly once!");

  if(task->state == TASK_STATE_READY)
    return ERR_TASK_READY;
  if(queue_add(&task_queues[task->priority], (void*) &task))
    return ERR_QUEUE_FULL;

  active_queues |= (0x80000000 >> task->priority);
  task->state = TASK_STATE_READY;
  return 0;
}

/*  Returns a task and removes it from the scheduler, assumes that the caller */
/*  will run the given task and reschedule it if necessary. */

struct task *sched_consume_task() {
  struct task *task;
  int priority;

  kassert(init == 1, "init_sched must be called exactly once!");
  if(active_queues == 0) return NULL;

  priority = clz(active_queues);
  queue_consume(&task_queues[priority], &task);

  if(task_queues[priority].empty)
    active_queues &= ~(0x80000000 >> task->priority);
    
  task->state = TASK_STATE_ACTIVE;
  return task;
}

/*  Return the next scheduled task without removing it from the scheduler. */

struct task *sched_next_task() {
  struct task *task;
  int priority;

  kassert(init == 1, "init_sched must be called exactly once!");
  if(active_queues == 0) return NULL;

  priority = clz(active_queues);
  queue_peek(&task_queues[priority], &task);
  return task;
}
