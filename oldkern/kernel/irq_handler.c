#include "emul/ts7200.h"
#include "kernel/irq_handler.h"
#include "kernel/constants.h"
#include "kernel/status.h"
#include "kernel/scheduler.h"
#include "bwio/bwio.h"
#include "common/assert.h"
#include "common/algs.h"

void unblock_awaiting_tasks(
  struct TaskDescriptor tds[],
  struct Scheduler *scheduler,
  struct AwaitedEvents *awaited_events,
  int eventid,
  int return_val
) {
  struct TaskDescriptor *current = awaited_events->priority_list_heads[eventid];
  struct TaskDescriptor *next = NULL;

  if (current == NULL && read_debug_timer() > 1000000) {
    awaited_events->num_missed_interrupts++;
  }

  while (current != NULL) {
    next = current->next;
    ASSERT(current->status == TASK_STATE_AWAIT_BLOCKED, "Attempting to unblock a task not in AWAIT_BLOCKED state: %d", current->tid);
    current->return_val = return_val;
    scheduler_add_td(scheduler, current);
    current = next;
  }

  awaited_events->priority_list_heads[eventid] = NULL;
  awaited_events->priority_list_tails[eventid] = NULL;
}

void handle_irq(
  struct TaskDescriptor tds[],
  struct Scheduler *scheduler,
  struct AwaitedEvents *awaited_events,
  int irq_1_status,
  int irq_2_status
) {
  int eventid = -1;
  int return_val = 0; // the return value of the AwaitEvent

  if (irq_2_status & VIC2_TIMER3_INTERRUPT) {
    eventid = EVENT_TIMER_TICK;

    volatile int *timer_clear = (int *)(TIMER3_BASE + CLR_OFFSET);
    *timer_clear = 1;
  } else if (irq_2_status & VIC2_UART1_INTERRUPT) {
    volatile int *uart_int = (int *)(UART1_BASE + UART_INTR_OFFSET);
    volatile int *flags = (int *)( UART1_BASE + UART_FLAG_OFFSET );
    volatile int *status = (int *)( UART1_BASE + UART_RSR_OFFSET );

    ASSERT((*status & 0xf) == 0, "Error bit set on COM1: %d", *status & 0xf);
    if (*uart_int & UART_TIS_MASK) {
      eventid = EVENT_UART1_XMIT_RDY;
      // disable the interrupt for now. re-enabled by handler server
      ASSERT(*flags & TXFE_MASK, "UART1 Transmit buffer full, but raised exception");

      int *uart_int_reg = (int *)(UART1_BASE + UART_CTLR_OFFSET);
      *uart_int_reg &= ~TIEN_MASK;
    } else if (*uart_int & UART_RIS_MASK) {
      eventid = EVENT_UART1_RCV_RDY;

      volatile int *data = (int *)( UART1_BASE + UART_DATA_OFFSET );
      ASSERT(*flags & RXFF_MASK, "UART1 Receive buffer not full, but raised exception");

      return_val = (int) *data;
    } else if (*uart_int & UART_MIS_MASK) {
      // MODEM STATUS INTERRUPT
      eventid = EVENT_UART1_MODEM;

      // clear interrupt
      *uart_int = 0;
    }
  } else if (irq_2_status & VIC2_UART2_INTERRUPT) {
    volatile int *uart_int = (int *)(UART2_BASE + UART_INTR_OFFSET);
    volatile int *flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
    volatile int *status = (int *)( UART2_BASE + UART_RSR_OFFSET );

    ASSERT((*status & 0xf) == 0, "Error bit set on COM2: %d", *status & 0xf);
    if (*uart_int & UART_TIS_MASK) {
      eventid = EVENT_UART2_XMIT_RDY;
      // disable the interrupt for now. re-enabled by handler server
      ASSERT(*flags & TXFE_MASK, "UART2 Transmit buffer full, but raised exception");

      int *uart_int_reg = (int *)(UART2_BASE + UART_CTLR_OFFSET);
      *uart_int_reg &= ~TIEN_MASK;
    } else if (*uart_int & UART_RIS_MASK) {
      eventid = EVENT_UART2_RCV_RDY;

      volatile int *data = (int *)( UART2_BASE + UART_DATA_OFFSET );
      ASSERT(*flags & RXFF_MASK, "UART2 Receive buffer not full, but raised exception");

      return_val = (int) *data;
    } else {
      ASSERT(0, "Unsupported UART_2 INT: %x", *uart_int);
    }
  } else {
    ASSERT(0, "Unsupported IRQ: %x-%x", irq_1_status, irq_2_status);
  }

  unblock_awaiting_tasks(
    tds,
    scheduler,
    awaited_events,
    eventid,
    return_val
  );
}
