#include <common/syscall.h>
#include <common/err.h>
#include <kern.h>
#include <sched.h>
#include <event.h>
#include <ipc.h>
#include <sched.h>
#include <task.h>
#include <common/uart.h>

static int init = 0;
/*  Contract: SWI receives a pointer to a proc_state object corresponding to the */
/*  processor state when the swi instruction was called. Similarly it returns a */
/*  proc_state object which describes the desired state of the world after the */
/*  function terminates. The plumbing for this function is done in the asm */
/*  function _swi_hander in swi.s. */

proc_state_t *swi(proc_state_t *state, int syscall) {
  dbg_noprefix("===================================KERN (SWI)===========================================");
  dbg("processing syscall of type %s for %d", syscall2str[syscall], kern_current_task->td);

  static proc_state_t *exit_proc_state;
  static task_t *task_set[MAX_TASKS] = {NULL};

  enum event ev;
  task_t *task;
  unsigned int priority;

  if(kern_current_task)
    kern_current_task->cpu_state = state;

  switch(syscall) {
    case SYSCALL_SEND: 
      if(kern_current_task->cpu_state->r0 < 0 ||
         kern_current_task->cpu_state->r0 >= MAX_TASKS ||
         !task_set[kern_current_task->cpu_state->r0]) {

        RETURN(kern_current_task, ERR_SEND_TARGET_DOES_NOT_EXIST);
      } else {
        ipc_send(kern_current_task, task_set[kern_current_task->cpu_state->r0]);
      }

      break;

    case SYSCALL_RECEIVE: 
      ipc_receive(kern_current_task); 
      break;

    case SYSCALL_REPLY: 
      ipc_reply(kern_current_task, task_set[kern_current_task->cpu_state->r0]);
      break;

    case SYSCALL_MYTID: 
      RETURN(kern_current_task, kern_current_task->td);
      break;

    case SYSCALL_CREATE:
      priority = (unsigned int) kern_current_task->cpu_state->r0;

      if(priority < 0 || priority > 31) {
        RETURN(kern_current_task, ERR_INVALID_PRIORITY);
        break;
      }

      task = task_create((void (*)()) kern_current_task->cpu_state->r1,
                         priority,
                         kern_current_task->td);

      task_set[task->td] = task;
      sched_add_task(task);
      RETURN(kern_current_task, task->td);
      break;
    case SYSCALL_AWAIT_EVENT:
    ev = kern_current_task->cpu_state->r0;
    event_subscribe(kern_current_task, ev);

    /* Additional event-specific logic */
    switch(ev) {
      case EVENT_UART1_TX:
        ua_txint_enable(COM1);
        break;
      case EVENT_UART2_TX:
        ua_txint_enable(COM2);
        break;
      case EVENT_UART1_CTS:
        if(kern_cts1 == 2) {
          event_notify(EVENT_UART1_CTS, 0, NULL, 0);
          kern_cts1 = 0;
        }
        break;
      case EVENT_UART2_CTS:
        if(kern_cts2 == 2) {
          event_notify(EVENT_UART2_CTS, 0, NULL, 0);
          kern_cts2 = 0;
        }
        break;
      default:
        break;
    }

      break;
    case SYSCALL_PARENT_TID:
      RETURN(kern_current_task, kern_current_task->ptd);
      break;
    case SYSCALL_PASS: 
      sched_add_task(kern_current_task);
      break;
    case SYSCALL_INITIALIZATION: 
      kassert(++init == 1, "SYSCALL_INITIALIZATION CALLED BY %d", kern_current_task->td);
      task_set[0] = sched_next_task();
      exit_proc_state = state;
      exit_proc_state->psr &= ~0x1F;
      exit_proc_state->psr |= 0xD3;
      break;
    case SYSCALL_EXIT:
      task_set[kern_current_task->td] = NULL;
      task_destroy(kern_current_task);
      break;
    case SYSCALL_TERMINATE: /* Terminate the entire system, useful for debugging */
      /* FIXME restore state of the box */
      bwprintf(COM2, "KERNEL TERMINATION REQUESTED BY %d\r\n", kern_current_task->td);
      return exit_proc_state;
      break;

    /* Blocking print, useful for debugging */
    case SYSCALL_PRINT:
      bwprintf(COM2, "TASK %d: ", kern_current_task->td);
      bwformat(COM2, kern_current_task->cpu_state->r0,
               kern_current_task->cpu_state->r1);
      return kern_current_task->cpu_state;
      break;
  }

  /* FIXME could end up with several blocked but unterminated tasks (is it worth distinguishing between the two?) */
  kern_current_task = sched_consume_task(); 
  /* Note when the task returns it will be unscheduled, it must be explicitly scheduled if desired */

  if(!kern_current_task) {
    bwprintf(COM2, "NO MORE TASKS (is idle running?)\r\n");
    while(1){}
  }

  dbg_noprefix("=================================USER (task %d)==========================================", kern_current_task->td);

  return kern_current_task->cpu_state;
}
