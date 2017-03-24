#include <task.h>
#include <arm_v4.h>
#include <common/syscall.h>
#include <common/ds/queue.h>

static int init = 0;
static queue_t free;
static task_t tasks[MAX_TASKS];
static base_sp; /* The start of user stack-space */
#define MEM_SZ 15000000 /* Size of memory reserved for user space. */

static void terminate() {
  SYSCALL(SYSCALL_EXIT);
}

void init_task() {
  int i;
  static task_t *t;
  static task_t *mem[MAX_TASKS];

  FETCH_REGISTER(base_sp, sp);
  /* Reserve 200 KB of space for the kernel */
  base_sp -= 200000;

  queue_init(&free, mem, sizeof(mem), sizeof(mem[0]));
  for (i = 0; i < MAX_TASKS; i++) {
    tasks[i].td = i;
    t = &tasks[i];
    queue_add(&free, &t);
  }
}

task_t *task_create(void (*start)(), unsigned int priority, int parent) {
  kassert(!init, "init_task must be called first");
  /* FIXME intelligently allocate stack space */
  int r, sp;
  task_t *task;

  r = queue_consume(&free, &task);
  kassert(!r, "Task queue is empty");
  sp = base_sp - ((MEM_SZ / MAX_TASKS) * task->td);

  task->priority = priority;
  task->cpu_state = (proc_state_t*) (sp - sizeof(proc_state_t));
  task->cpu_state->psr = 0x50; /* USER mode with FIQ disable and IRQ enabled. */
  task->cpu_state->pc = (int)start;
  task->cpu_state->lr = (int)terminate;
  task->cpu_state->sp = (int)sp;
  task->state = TASK_STATE_NEW;
  task->ptd = parent;

  return task;
}

int task_destroy(task_t *task) {
  kassert(!init, "init_task must be called first");
  /* TODO free associated memory */
  queue_add(&free, &task);
  task->state = TASK_STATE_TERMINATED;
  return 0;
}
