#include <task.h>
#include <arm_v4.h>
#include <common/syscall.h>

static task_t tasks[MAX_TASKS];
static int td = 0; 

static void terminate() {
  SYSCALL(SYSCALL_EXIT);
}

task_t *task_create(void (*start)(), unsigned int priority, int parent) {
  /* FIXME intelligently alloate stack space */
  static char *sp = 0;

  if(!sp) FETCH_REGISTER(sp, sp);

  sp -= 200000; /* hand out 200 KB stack segments :/ */

  tasks[td].priority = priority;
  tasks[td].cpu_state = (proc_state_t*) (sp - sizeof(proc_state_t));
  tasks[td].cpu_state->psr = 0x50; /* USER mode with FIQ disable and IRQ enabled. */
  tasks[td].cpu_state->pc = (int)start;
  tasks[td].cpu_state->lr = (int)terminate;
  tasks[td].cpu_state->sp = (int)sp;
  tasks[td].td = td;
  tasks[td].state = TASK_STATE_NEW;
  tasks[td].ptd = parent;
  td++; /* FIXME assign/free task descriptors intelligently */

  return &tasks[td-1];
}

int task_destroy(task_t *task) {
  /* TODO free associated memory */
  task->state = TASK_STATE_TERMINATED;
  return 0;
}
