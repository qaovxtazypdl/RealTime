#include <kern.h>
#include <common/uart.h>
#include <common/timer.h>
#include <sched.h>
#include <event.h>
enum interrupt {
  INT_TIMER,
  INT_UART2_RX,
  INT_UART2_TX,
  INT_UART1_RX,
  INT_UART1_TX,
  INT_UART1_CTS,
  INT_UART2_CTS,
  INT_UNKNOWN /* should not happen */
};

static enum interrupt inline interrupt_type() {
  volatile int icu_status = *((int*)(VIC2_BASE + VIC_IRQ_STATUS_OFFSET));
  volatile int base;

  if (icu_status & VIC_TIMER3_EN)
    return INT_TIMER;

  if(icu_status & (VIC_UART1_EN | VIC_UART2_EN)) {
    base = (icu_status & VIC_UART1_EN) ? UART1_BASE : UART2_BASE;

    if (*((int*)(base + UART_INTR_OFFSET)) & MDM_INTR)
      return (base == UART2_BASE) ? INT_UART2_CTS : INT_UART1_CTS;
    else if(*((int*)(base + UART_INTR_OFFSET)) & TX_INTR)
      return (base == UART2_BASE) ? INT_UART2_TX : INT_UART1_TX;
    else if (*((int*)(base + UART_INTR_OFFSET)) & RX_INTR)
      return (base == UART2_BASE) ? INT_UART2_RX : INT_UART1_RX;
  }

  return INT_UNKNOWN;
}

/* Responsible for clearing active interrupts to prevent immediate re-entry */

proc_state_t *irq(proc_state_t *state) {
  enum interrupt type = interrupt_type();
  char c;

  switch(type) {
    case INT_TIMER:
      timer_clear_interrupt(); 
      event_notify(EVENT_TIMER_INTERRUPT, 0, NULL, 0);
      break;
    case INT_UART2_RX:
      c = *((char*)(UART2_BASE + UART_DATA_OFFSET));
      event_notify(EVENT_UART2_RX, 0, (void*)&c, sizeof(c));
      break;
    case INT_UART1_RX:
      c = *((char*)(UART1_BASE + UART_DATA_OFFSET));
      event_notify(EVENT_UART1_RX, 0, (void*)&c, sizeof(c));
      break;
    case INT_UART2_TX:
      event_notify(EVENT_UART2_TX, 0, NULL, 0);
      ua_txint_disable(COM2);
      break;
    case INT_UART1_TX:
      event_notify(EVENT_UART1_TX, 0, NULL, 0);
      ua_txint_disable(COM1);
      break;
    case INT_UART1_CTS:
      ua_clear_cts(COM1);
      if((++kern_cts1 == 2) && 
          !event_notify(EVENT_UART1_CTS, 0, NULL, 0))
        kern_cts1 = 0;
      break;
    case INT_UART2_CTS:
      ua_clear_cts(COM2);
      if((++kern_cts2 == 2) && 
          !event_notify(EVENT_UART2_CTS, 0, NULL, 0))
        kern_cts2 = 0;
      break;
    default:
      kassert(0, "A WILD INTERRUPT HAS APPEARED!, VIC2_STAT: %x, VIC1_STAT: %x", 
          *((int*)(VIC2_BASE + VIC_IRQ_STATUS_OFFSET)), 
          *((int*)(VIC1_BASE + VIC_IRQ_STATUS_OFFSET)));
      break;
  }

  kern_current_task->cpu_state = state;
  sched_add_task(kern_current_task);

  kern_current_task = sched_consume_task();
  return kern_current_task->cpu_state;
}
