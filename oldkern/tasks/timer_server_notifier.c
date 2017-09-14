#include "kernel/syscall.h"
#include "common/algs.h"
#include "tasks/utils/ns_names.h"
#include "tasks/timer_server_notifier.h"
#include "tasks/name_server.h"
#include "bwio/bwio.h"
#include "common/constants.h"
#include "common/assert.h"

// TODO: error handling (returning and checking)

/*
0 Success.
-1 The clock server task id is invalid.
-2 The delay was zero or negative.
*/
int Delay(int tid, int ticks) {
  if (ticks <= 0) {
    return -2;
  }

  int result = -1;
  int reply = -1;
  struct TimerServerRequest request;
  request.type = TIMERSERVER_DELAY;
  request.ticks = ticks;

  result = Send(tid, (char *) &request, sizeof(struct TimerServerRequest), (char *) &reply, 4);

  if (result < 0 || reply < 0) {
    return -1;
  } else {
    return 0;
  }
}

/*
0 Success.
-1 The clock server task id is invalid.
-2 The delay was zero or negative.
*/
int DelayUntil(int tid, int ticks) {
  int result = -1;
  int reply = -1;
  struct TimerServerRequest request;
  request.type = TIMERSERVER_DELAYUNTIL;
  request.ticks = ticks;

  result = Send(tid, (char *) &request, sizeof(struct TimerServerRequest), (char *) &reply, 4);

  if (result == -2) {
    return -2;
  } else if (result < 0 || reply < 0) {
    return -1;
  } else {
    return 0;
  }
}

/*
>-1 The time in ticks since the clock server initialized.
-1 The clock server task id is invalid.
*/
int Time(int tid) {
  int result = -1;
  int reply = -1;
  struct TimerServerRequest request;
  request.type = TIMERSERVER_TIME;

  result = Send(tid, (char *) &request, sizeof(struct TimerServerRequest), (char *) &reply, 4);

  if (result < 0 || reply < 0) {
    return -1;
  } else {
    return reply;
  }
}

int NotifyTick(int tid) {
  int result = -1;
  int reply = -1;
  struct TimerServerRequest request;
  request.type = TIMERSERVER_NOFITYTICK;

  result = Send(tid, (char *) &request, sizeof(struct TimerServerRequest), (char *) &reply, 4);

  if (result < 0 || reply < 0) {
    return -1;
  } else {
    return 0;
  }
}

// the data structure holds the smallest element at the highest index.
struct TimerPriorityQueue {
  int priorities[NUM_TASKS_MAX];
  char tids[NUM_TASKS_MAX];
  int length;
};

// O(n) push by insertion
int delay_queue_push(struct TimerPriorityQueue *delay_queue, int priority, char tid) {
  if (delay_queue->length >= NUM_TASKS_MAX) {
    return -1;
  }

  // add to front of queue
  delay_queue->priorities[delay_queue->length] = priority;
  delay_queue->tids[delay_queue->length] = tid;
  delay_queue->length++;

  // insert into proper place
  int insertion_index = delay_queue->length - 1;
  int temp_priority;
  int temp_tid;
  while (
    insertion_index > 0 &&
    delay_queue->priorities[insertion_index] >= delay_queue->priorities[insertion_index - 1]
  ) {
    temp_priority = delay_queue->priorities[insertion_index];
    temp_tid = delay_queue->tids[insertion_index];

    delay_queue->priorities[insertion_index] = delay_queue->priorities[insertion_index - 1];
    delay_queue->tids[insertion_index] = delay_queue->tids[insertion_index - 1];

    delay_queue->priorities[insertion_index - 1] = temp_priority;
    delay_queue->tids[insertion_index - 1] = temp_tid;
    insertion_index--;
  }
  return 0;
}

// constant time pop
int delay_queue_pop(struct TimerPriorityQueue *delay_queue, int *priority, char *tid) {
  if (delay_queue->length <= 0) {
    return -1;
  }

  delay_queue->length--;
  *priority = delay_queue->priorities[delay_queue->length];
  *tid = delay_queue->tids[delay_queue->length];

  if (delay_queue->length > 0) {
    ASSERT(delay_queue->priorities[delay_queue->length - 1] >= *priority,
      "TimerServer - delay queue out of order! %d < %d",
      delay_queue->priorities[delay_queue->length - 1],
      *priority
    );
  }
  return 0;
}

// constant time peek
int delay_queue_peek(struct TimerPriorityQueue *delay_queue, int *priority, char *tid) {
  if (delay_queue->length <= 0) {
    return -1;
  }

  *priority = delay_queue->priorities[delay_queue->length - 1];
  *tid = delay_queue->tids[delay_queue->length - 1];
  return 0;
}

void task__timer_server() {
  int ticks = 0;
  int retval;
  retval = RegisterAs(TIMERSERVER_NS_NAME);

  if (retval < 0) {
    bwprintf(COM2, "TimerServer - RegisterAs failed: %d!\n\r", retval);
  }

  int tid = 0;
  struct TimerServerRequest request;
  int reply = -1;

  struct TimerPriorityQueue delay_queue;
  delay_queue.length = 0;

  int priority_result;
  char tid_result;
  for (;;) {
    retval = Receive(&tid, (char *) &request, sizeof(struct TimerServerRequest));
    if (retval < 0) {
      bwprintf(COM2, "TimerServer - Receive failed: %d!\n\r", retval);
    }
    switch (request.type) {
      case TIMERSERVER_NOFITYTICK:
        ticks++;

        // first reply to the notifier.
        reply = 0;
        retval = Reply(tid, (char *) &reply, 4);

        // then unblock tasks if necessary
        while (
          delay_queue_peek(&delay_queue, &priority_result, &tid_result) == 0 &&
          ticks >= priority_result
        ) {
          delay_queue_pop(&delay_queue, &priority_result, &tid_result);

          reply = 0;
          retval = Reply(tid_result, (char *) &reply, 4);
        }
        break;
      case TIMERSERVER_TIME:
        reply = ticks;
        retval = Reply(tid, (char *) &reply, 4);
        break;
      case TIMERSERVER_DELAY:
        // does not reply
        delay_queue_push(&delay_queue, ticks + request.ticks, tid);
        break;
      case TIMERSERVER_DELAYUNTIL:
        // does not reply if valid
        if (ticks >= request.ticks) {
          reply = -2;
          retval = Reply(tid, (char *) &reply, 4);
        } else {
          delay_queue_push(&delay_queue, request.ticks, tid);
        }
        break;
      default:
        reply = -4;
        retval = Reply(tid, (char *) &reply, 4);
        break;
    }
    if (retval < 0) {
      bwprintf(COM2, "TimerServer - Reply failed: %d!\n\r", retval);
    }
  }
}

void task__timer_notifier() {
  int timer_server_tid;
  timer_server_tid = WhoIs(TIMERSERVER_NS_NAME);

  if (timer_server_tid < 0) {
    bwprintf(COM2, "Timer Notifier - Server ID not found: %d.\n\r", timer_server_tid);
  }

  int retval = 0;
  for (;;) {
    retval = AwaitEvent(EVENT_TIMER_TICK);
    if (retval < 0) {
      bwprintf(COM2, "TimerServer - AwaitEvent failed: %d!\n\r", retval);
    }
    retval = NotifyTick(timer_server_tid);
    if (retval < 0) {
      bwprintf(COM2, "TimerServer - NotifyTick failed: %d!\n\r", retval);
    }
  }
}
