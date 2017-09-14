#include "kernel/syscall.h"
#include "common/constants.h"
#include "bwio/bwio.h"
#include "emul/ts7200.h"
#include "tasks/timer_server_notifier.h"
#include "tasks/utils/ns_names.h"

/*
A single type of client task tests your kernel and clock server. It is created by the first
user task, and immediately sends to its parent, the first user task, requesting a delay
time, t, and a number, n, of delays. It then uses WhoIs to discover the tid of the clock
server.
It then delays n times, each time for the time interval, t. After each delay it prints
its tid, its delay interval, and the number of delays currently completed on the terminal
connected to the ARM box.
*/

struct ClockServerArgumentReceiver {
  int num_ticks;
  int num_delays;
};

void task__test_clockserver() {
  int parent_tid =  MyParentTid();
  int my_tid =  MyTid();
  int retval = 0;
  int timer_server_tid = WhoIs(TIMERSERVER_NS_NAME);
  if (timer_server_tid < 0) {
    bwprintf(COM2, "%d - Timer Server ID not found: %d.\n\r", my_tid, timer_server_tid);
  }

  bwprintf(COM2, "%d - init\n\r", my_tid);

  struct ClockServerArgumentReceiver args;
  char my_tid_buf = my_tid;

  retval = Send(parent_tid, &my_tid_buf, 1, (char *)&args, sizeof(struct ClockServerArgumentReceiver));
  if (retval < 0) {
    bwprintf(COM2, "%d - send failed: %d.\n\r", my_tid, retval);
  }

  int iterator;
  for (iterator = 0; iterator < args.num_delays; iterator++) {
    retval = Delay(timer_server_tid, args.num_ticks);
    if (retval < 0) {
      bwprintf(COM2, "%d - delay failed: %d.\n\r", my_tid, retval);
    }
    bwprintf(COM2, "tid-%d delay-%d iter-%d\n\r", my_tid, args.num_ticks, iterator + 1);
  }

  Exit();
}
