#include "tasks/idle.h"
#include "emul/ts7200.h"
#include "bwio/bwio.h"
#include "kernel/syscall.h"
#include "common/algs.h"
#include "tasks/name_server.h"
#include "tasks/utils/ns_names.h"
#include "tasks/utils/printf.h"

/*
void task__idle() {
  volatile int *device_config_register = DVC_CONFIG;
  volatile int *halt = HALT_REGISTER;
  *device_config_register |= SH_ENA_MASK;
  int tid = MyTid();
  int read_reg = 0;

  for (;;) {
    // read from halt register
    read_reg = *halt;
  }
  Exit();
  return read_reg;
}
*/
#define DEBUG_TIMER_INTERVAL (98304 * 5) //500ms

// use the debug timer
void task__idle() {
  int last_print = read_debug_timer();
  int timer = last_print;
  int busyloop_it = 0;
  int ret = 0;

  int print_server_tid = WhoIs(UART2_XMIT_SERVER_NS_NAME);
  if (print_server_tid < 0) {
    bwprintf(COM2, "Idle task - NS error: %d\n\r", print_server_tid);
  }

  int last_my_time_total = 0, last_total_ticks_elapsed = 0;
  int my_time_total = 0, total_ticks_elapsed = 0;
  int min_last_interval_idle = 10000;
  for (;;) {
    if (timer > DEBUG_TIMER_INTERVAL * 2 && timer - last_print > DEBUG_TIMER_INTERVAL) {
      MyProfilerTime(&my_time_total, &total_ticks_elapsed);
      int percent_idle_last100ms = (int) (
        (double) (my_time_total - last_my_time_total) /
        (double) (total_ticks_elapsed - last_total_ticks_elapsed) * 10000.0
      );

      int percent_idle_cumulative = (int) (
        (double) my_time_total /
        (double) total_ticks_elapsed * 10000.0
      );

      if (min_last_interval_idle > percent_idle_last100ms) {
        min_last_interval_idle = percent_idle_last100ms;
      }

      ret = task_print_idle(print_server_tid, percent_idle_cumulative, percent_idle_last100ms, min_last_interval_idle);
      if (ret < 0) {
        bwprintf(COM2, "Idle task - print error: %d\n\r", ret);
      }

      last_my_time_total = my_time_total;
      last_total_ticks_elapsed = total_ticks_elapsed;
      last_print = timer;
    }

    // slow down reads a bit
    busyloop_it = 0;
    while (busyloop_it < 1000) {
      busyloop_it++;
    }
    timer = read_debug_timer();
  }
  Exit();
}
