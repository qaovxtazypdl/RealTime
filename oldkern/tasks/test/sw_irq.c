#include "kernel/syscall.h"
#include "common/constants.h"
#include "bwio/bwio.h"
#include "emul/ts7200.h"
#include "tasks/timer_server_notifier.h"

int swirq_helper() {
  int tid =  MyTid();
  bwprintf(COM2, "%d: sw_irq - helper entry\n\r", tid);

  bwprintf(COM2, "%d: timer ticked - ret:%d\n\r", tid, AwaitEvent(EVENT_TIMER_TICK));
  bwprintf(COM2, "%d: timer ticked - ret:%d\n\r", tid, AwaitEvent(EVENT_TIMER_TICK));
  bwprintf(COM2, "%d: timer ticked - ret:%d\n\r", tid, AwaitEvent(EVENT_TIMER_TICK));
  bwprintf(COM2, "%d: timer ticked - ret:%d\n\r", tid, AwaitEvent(EVENT_TIMER_TICK));
  bwprintf(COM2, "%d: timer ticked - ret:%d\n\r", tid, AwaitEvent(EVENT_TIMER_TICK));
  bwprintf(COM2, "%d: timer ticked - ret:%d\n\r", tid, AwaitEvent(EVENT_TIMER_TICK));
  bwprintf(COM2, "%d: timer ticked - ret:%d\n\r", tid, AwaitEvent(-23));
  bwprintf(COM2, "%d: timer ticked - ret:%d\n\r", tid, AwaitEvent(454));

  bwprintf(COM2, "%d: sw_irq - helper exit\n\r", tid);
  return 0x12345;
}

//void task__first() {
void task__test_sw_irq() {
  //bwprintf(COM2, "give me the time: %d\n\r", Time(2));

  int tid =  MyTid();
  int ret = 0;
  bwprintf(COM2, "%d: sw_irq - starting\n\r", tid);
  ret = swirq_helper();
  bwprintf(COM2, "%d: sw_irq - ending: %x\n\r", tid, ret);

  //bwprintf(COM2, "give me the time: %d\n\r", Time(2));
  Exit();
}
