#include <common/ds/queue.h>
#include <common/mem.h>
#include <common/event.h>
#include <event.h>
#include <task.h>
#include <sched.h>

#define EVENT_QUEUE_SZ MAX_TASKS

static queue_t event_queues[NUM_EVENT_TYPES];
static int init = 0;

void init_event() {
  int i;

  init++;

  static struct task* queue_mem[NUM_EVENT_TYPES][EVENT_QUEUE_SZ];
 
  for (i = 0; i < NUM_EVENT_TYPES; i++)
    queue_init(&event_queues[i],
               queue_mem[i],
               sizeof(queue_mem[0]),
               sizeof(queue_mem[0][0]));
}

int event_subscribe(struct task *task, enum event type) {
  int full;
  kassert(init == 1, "init_event must be called exactly once");
  kassert(type < NUM_EVENT_TYPES && type >= 0, "invalid event type");

  task->state = TASK_STATE_EVENT_BLOCKED;
  full = queue_add(&event_queues[type], &task);

  kassert(!full, "Event queue for %s is full!", event2str[type]);
  return 0;
}

/* Returns 0 if at least one task was waiting on the event */
int event_notify(enum event type, int retval, void *payload, int payload_sz) { 
  struct task *task = NULL;

  kassert(init == 1, "init_event must be called exactly once");

  while(!queue_consume(&event_queues[type], &task)) {
    memcpy((void*)task->cpu_state->r1, payload, payload_sz);
    task->cpu_state->r0 = retval;
    sched_add_task(task);
  }

  return (task == NULL);
}
