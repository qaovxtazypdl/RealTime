#include "kernel/syscall.h"
#include "common/algs.h"
#include "kernel/constants.h"
#include "bwio/bwio.h"

#define NUM_ITERATIONS 10000

void task__timed_producer() {
  int tid = -1;
  int rcvlen = -11;
  int rplsts = -11;
  char msgbuf[TEST_MESSAGE_BYTES], replybuf[TEST_MESSAGE_BYTES];

  int iterations;
  for (iterations = 0; iterations < NUM_ITERATIONS; iterations++) {
    rcvlen = Receive(&tid, msgbuf, TEST_MESSAGE_BYTES);
    rplsts = Reply(tid, replybuf, TEST_MESSAGE_BYTES);
  }

  Exit();
}

void task__timed_consumer() {
  char msgbuf[TEST_MESSAGE_BYTES], replybuf[TEST_MESSAGE_BYTES];
  int rcvlen = 0;
  int time_ticks_before = 0;
  int time_ticks_after = 0;

  unsigned int total_ticks = 0;
  unsigned int max_ticks = 0;
  unsigned int min_ticks = 0xffffffff;
  unsigned int iterations = 0;
  unsigned int current_ticks = 0;
  for (iterations = 0; iterations < NUM_ITERATIONS; iterations++) {
    if (iterations % 1000 == 0) {
      bwprintf(COM2, "iteration: %d\n\r", iterations);
    }

    time_ticks_before = read_debug_timer();
    rcvlen = Send(8, msgbuf, TEST_MESSAGE_BYTES, replybuf, TEST_MESSAGE_BYTES);
    time_ticks_after = read_debug_timer();

    if (rcvlen != TEST_MESSAGE_BYTES) {
      bwprintf(COM2, "send failed: %d!\n\r", rcvlen);
    }

    current_ticks = time_ticks_after - time_ticks_before;
    if (current_ticks > max_ticks) {
      max_ticks = current_ticks;
    }
    if (current_ticks < min_ticks) {
      min_ticks = current_ticks;
    }
    total_ticks += current_ticks;
  }


  bwprintf(COM2, "========RESULTS=======\n\r");
  bwprintf(COM2, "iterations = %d\n\r", NUM_ITERATIONS);
  bwprintf(COM2, "total ticks = %d\n\r", total_ticks);
  bwprintf(COM2, "avg ticks = %d\n\r", (total_ticks/NUM_ITERATIONS));
  bwprintf(COM2, "max ticks = %d\n\r", max_ticks);
  bwprintf(COM2, "min ticks = %d\n\r", min_ticks);

  Exit();
}

void simple_timed_syscall_test() {
  int retval;
  bwprintf(COM2, "FUT - Starting!\n\r");

  int consumer_priority;
  if (SEND_BEFORE_REPLY) {
    consumer_priority = 20;
  } else {
    consumer_priority = 22;
  }

  retval = Create(21, &task__timed_producer);
  bwprintf(COM2, "FUT - Producer id %d\n\r", retval);

  retval = Create(consumer_priority, &task__timed_consumer);
  bwprintf(COM2, "FUT - Consumer id %d\n\r", retval);

  Exit();
}
