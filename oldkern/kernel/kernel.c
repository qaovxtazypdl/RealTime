#include "emul/ts7200.h"
#include "bwio/bwio.h"
#include "kernel/context_switch.h"
#include "kernel/irq_handler.h"
#include "kernel/error.h"
#include "kernel/status.h"
#include "common/assert.h"
#include "kernel/syscall.h"
#include "tasks/first.h"
#include "common/algs.h"
#include "common/byte_buffer.h"
#include "kernel/kernel_ds.h"
#include "kernel/scheduler.h"
#include "kernel/syscall_handlers.h"

int kernel_schedule(struct Scheduler *scheduler) {
  if (scheduler->nonempty_queue_bitmask == 0) {
    return -1;
  }
  int priority;
  priority = clz(scheduler->nonempty_queue_bitmask, scheduler->clz_lookup_map);
  if (scheduler->active->status == TASK_STATE_ACTIVE) {
    scheduler->active->status = TASK_STATE_READY;
  }

  // if idle task scheduled and shudown  requested, shutdown.
  if (scheduler->flag_shutdown && priority == NUM_SCHEDULER_PRIORITIES - 1) {
    return -1;
  }

  ASSERT(scheduler->priority_list_heads[priority] != NULL, "Scheduled task was NULL.");

  scheduler->active = scheduler->priority_list_heads[priority];
  scheduler->active->status = TASK_STATE_ACTIVE;

  return (int) scheduler->active->tid;
}

void kernel_handle(
  struct TaskDescriptor tds[],
  struct KernelRequest *request,
  struct TaskAllocationData *alloc_data,
  struct Scheduler *scheduler,
  struct AwaitedEvents *awaited_events,
  struct ByteBuffer sendQueues[],
  struct MessageRequest messageRequests[],
  struct TaskProfilerData *profiler
) {
  switch (request->request_id) {
    case KERNEL_IRQ_ID:
      handle_irq(
        tds,
        scheduler,
        awaited_events,
        *((int *) (VIC1_BASE + VIC_IRQ_STATUS_OFFSET)),
        *((int *) (VIC2_BASE + VIC_IRQ_STATUS_OFFSET))
      );
      break;
    case SYSCALL_AWAIT_EVENT_ID:
      scheduler->active->return_val = syscall_await_event(
        tds,
        scheduler,
        awaited_events,
        request->arg0
      );
      break;
    case SYSCALL_CREATE_ID:
      scheduler->active->return_val = syscall_create(tds, alloc_data, scheduler, scheduler->active->tid, request->arg0, (void *)request->arg1);
      break;
    case SYSCALL_MY_TID_ID:
      scheduler->active->return_val = scheduler->active->tid;
      break;
    case SYSCALL_MY_PARENT_TID_ID:
      scheduler->active->return_val = scheduler->active->parent_tid;
      break;
    case SYSCALL_PASS_ID:
      scheduler_move_active_to_ready(scheduler);
      scheduler->active->return_val = 0;
      break;
    case SYSCALL_EXIT_ID:
      scheduler_remove_active_from_queue(scheduler);
      scheduler->active->status = TASK_STATE_ZOMBIE;
      scheduler->active->return_val = 0;
      break;
    case SYSCALL_SEND_ID:
      // blocking call - do not set return value here.
      scheduler->active->return_val = syscall_send(
        tds,
        alloc_data,
        scheduler,
        sendQueues,
        messageRequests,
        request->arg0, (char *) request->arg1, request->arg2, (char *) request->arg3, *(request->user_sp + 14 + 4) // 14 things on the stack, 4 offset from original stack pointer
      );
      break;
    case SYSCALL_RECEIVE_ID:
      // blocking call - do not set return value here.
      syscall_receive(
        tds,
        scheduler,
        sendQueues,
        messageRequests,
        (int *) request->arg0, (char *) request->arg1, request->arg2
      );
      break;
    case SYSCALL_REPLY_ID:
      scheduler->active->return_val = syscall_reply(
        tds,
        alloc_data,
        scheduler,
        sendQueues,
        messageRequests,
        request->arg0, (char *) request->arg1, request->arg2
      );
      break;
    case SYSCALL_MY_PROFILER_TIME:
      *((int *) request->arg0) = profiler->task_time[scheduler->active->tid];
      *((int *) request->arg1) = read_debug_timer() - profiler->profile_start_time;
      scheduler->active->return_val = 0;
      break;
    case SYSCALL_SHUTDOWN_ID:
      scheduler->flag_shutdown = TRUE;
      break;
    default:
      ASSERT(0, "INVALID SYSCALL: %d", request->request_id);
      scheduler->active->return_val = KERR_INVALID_SYSCALL;
  }
}

void kernel_initialize(
  struct TaskDescriptor *tds,
  struct TaskAllocationData *alloc_data,
  struct Scheduler *scheduler,
  struct AwaitedEvents *awaited_events,
  char sendQueueBuffer[],
  struct ByteBuffer sendQueues[],
  struct TaskProfilerData *profiler
) {
  __asm__("msr cpsr_c, #0xd3");

  __asm__("stmfd sp, {r0}");
  // invalidate data/instr caches
  __asm__("mov r0, #0");
  __asm__("mcr p15, 0, r0, c7, c7, 0");

  // initialize data/instr caches
  __asm__("mrc p15, 0, r0, c1, c0, 0");
#if CACHES_ON == 1
    __asm__("orr r0, r0, #0x1000");
    __asm__("orr r0, r0, #0x4");
#else
    __asm__("bic r0, r0, #0x1000");
    __asm__("bic r0, r0, #0x4");
#endif
  __asm__("mcr p15, 0, r0, c1, c0, 0");
  __asm__("ldmfd sp, {r0}");

  bwsetfifo(COM2, OFF);
  bwsetspeed(COM2, 115200);

  // set erntry point for both swi and irq
  int **entry = (int **) 0x28;
  *entry = (int *) ((int)&kernel_entry + 0x00218000);
  entry = (int **) 0x38;
  *entry = (int *) ((int)&kernel_entry_irq + 0x00218000);

  // allocation data struct
  alloc_data->next_stack_pointer = (int) USER_STACK_TOP;
  alloc_data->num_tasks_allocated = 0;

  int iterator;
  for (iterator = 0; iterator < NUM_TASKS_MAX; iterator++) {
    init_byte_buffer(
      sendQueues + iterator,
      sendQueueBuffer + (iterator * SEND_QUEUE_LENGTH),
      SEND_QUEUE_LENGTH
    );

    profiler->task_time[iterator] = 0;
  }
  profiler->profile_start_time = 0;
  profiler->kernel_time = 0;

  scheduler_init(scheduler);
  for (iterator = 0; iterator < NUM_IRQ_MAX; iterator++) {
    awaited_events->priority_list_heads[iterator] = NULL;
    awaited_events->priority_list_tails[iterator] = NULL;
  }
  awaited_events->num_missed_interrupts = 0;

  // init first user task at mid priority.
  syscall_create(tds, alloc_data, scheduler, 0, 15, &task__first);

  // populate active with the first element in tds array (newly created)
  scheduler->active = tds;

  // enable irq (hardware interrupts) relevant
  INTERRUPTS_OFF();
  INTERRUPTS_ON();

  int *uart1_int_reg = (int *)(UART1_BASE + UART_CTLR_OFFSET);
  *uart1_int_reg &= ~(MSIEN_MASK | RIEN_MASK | TIEN_MASK | RTIEN_MASK);

  int *uart2_int_reg = (int *)(UART2_BASE + UART_CTLR_OFFSET);
  *uart2_int_reg &= ~(MSIEN_MASK | RIEN_MASK | TIEN_MASK | RTIEN_MASK);

  // init timing clock
  volatile int *timer_ctrl, *timer_val, *timer_init;
  timer_init = (int *)(TIMER3_BASE + LDR_OFFSET);
  timer_val = (int *)(TIMER3_BASE + VAL_OFFSET);
  timer_ctrl = (int *)(TIMER3_BASE + CRTL_OFFSET);
  *timer_init = TIMER_3_FREQUENCY_CENTISECOND;
  *timer_ctrl = *timer_ctrl | ENABLE_MASK | CLKSEL_MASK | MODE_MASK;

  // init debug timer
  enable_debug_timer();
}

int main( int argc, char *argv[] ) {
  struct TaskDescriptor tds[NUM_TASKS_MAX];
  struct TaskAllocationData alloc_data;
  struct Scheduler scheduler;
  struct KernelRequest request;
  struct AwaitedEvents awaited_events;
  struct TaskProfilerData profiler;

  // 128 byte buffers of size 16 to hold each sender tid in queue
  char sendQueueBuffer[NUM_TASKS_MAX * 16];
  struct ByteBuffer sendQueues[NUM_TASKS_MAX];

  // array of 128 send requests
  struct MessageRequest messageRequests[NUM_TASKS_MAX];

  kernel_initialize(
    tds,
    &alloc_data,
    &scheduler,
    &awaited_events,
    sendQueueBuffer,
    sendQueues,
    &profiler
  );

  int debug_timer_val_1 = read_debug_timer();
  int debug_timer_val_2 = debug_timer_val_1;
  profiler.profile_start_time = debug_timer_val_1;
  int scheduled_tid;
  for (;;) {
    scheduled_tid = kernel_schedule(&scheduler);
    if (scheduled_tid == -1) {
      break;
    }

    debug_timer_val_1 = read_debug_timer();
    profiler.kernel_time += debug_timer_val_1 - debug_timer_val_2;
    kernel_exit(scheduler.active, &request);
    debug_timer_val_2 = read_debug_timer();
    profiler.task_time[scheduled_tid] += debug_timer_val_2 - debug_timer_val_1;

    kernel_handle(
      tds,
      &request,
      &alloc_data,
      &scheduler,
      &awaited_events,
      sendQueues,
      messageRequests,
      &profiler
    );
  }

  // a little bit of exit diagnostics
  int i = 0;
  for (i = 0; i < alloc_data.num_tasks_allocated; i++) {
    if (tds[i].status != TASK_STATE_ZOMBIE) {
      dbwprintf(COM2, "Task %d is not zombified!\n\r", i);
    }
  }
  dbwprintf(COM2, "missed %d interrupts\n\r", awaited_events.num_missed_interrupts);

  bwprintf(COM2, "\n\r\n\r");

  // disable irq (hardware interrupts)
  INTERRUPTS_OFF();

  int *uart1_int_reg = (int *)(UART1_BASE + UART_CTLR_OFFSET);
  *uart1_int_reg &= ~(MSIEN_MASK | RIEN_MASK | TIEN_MASK | RTIEN_MASK);

  int *uart2_int_reg = (int *)(UART2_BASE + UART_CTLR_OFFSET);
  *uart2_int_reg &= ~(MSIEN_MASK | RIEN_MASK | TIEN_MASK | RTIEN_MASK);

  return 0;
}
