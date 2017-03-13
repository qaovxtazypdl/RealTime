#include <common/ds/queue.h>
#include <common/syscall.h>
#include <common/types.h>
#include <clock_server.h>
#include "clock_server.h"
#include "../notifier.h"
#include <assert.h>
#include <ns.h>

#define MAX_DELAYED_TASKS 20

static int init = 0;
static int ctd = -1;
static void clock_server();

void init_clock_server() {
  assert(init == 0, "init_clock_server() must be called exactly once");
  ctd = create(0, clock_server);
  init++;
}

static void clock_server() {
  int td, r, full;
  int ctime = 0;
  queue_t delayed_tasks;
  struct clsv_msg msg;
  struct clsv_pending_task pt, start;
  struct clsv_pending_task delayed_tasks_mem[MAX_DELAYED_TASKS];

  int cn = create_notifier(0, EVENT_TIMER_INTERRUPT, NULL);
  queue_init(&delayed_tasks,
      delayed_tasks_mem,
      sizeof(delayed_tasks_mem),
      sizeof(delayed_tasks_mem[0]));

  while(1) {
    receive(&td, (void*)&msg, sizeof(msg));
    r = 0;

    if(td == cn) {
        reply(td, NULL, 0);
        /* FIXME this could overflow after an hour or so, depending on the granularity of the clock */
        ctime++; 

        /* FIXME a bit expensive and cumbersome, 
           but reliable (also makes an implicit assumption that requests are distinct) */
        if(!queue_consume(&delayed_tasks, &start)) { 
          queue_add(&delayed_tasks, &start);

          while(!queue_consume(&delayed_tasks, &pt)) {
            if(pt.time < ctime) {
              reply(pt.td, NULL, 0);
            } else {
              full = queue_add(&delayed_tasks, &pt);
              assert(!full, "clock queue full!");
            }

            if((start.td == pt.td) && (start.time == pt.time))
              break;
          }
        }
        continue;
    }

    switch(msg.op) {
      case CLOCK_SERVER_DELAY_REQUEST:
        pt.td = td;
        pt.time = msg.delay + ctime;
        full = queue_add(&delayed_tasks, &pt);
        assert(!full, "clock queue full!");
        break;
      case CLOCK_SERVER_DELAY_UNTIL_REQUEST:
        pt.td = td;
        pt.time = msg.delay; 
        full = queue_add(&delayed_tasks, &pt);
        assert(!full, "clock queue full!");
        break;
      case CLOCK_SERVER_GET_TIME_REQUEST:
        reply(td, &ctime, sizeof(ctime));
        break;
      default:
        break;
    }
  }
}
/* A courier which relays delay requests and notifies the parents when it is done via 
a send call. */

static void courier() {
  int td;
  int time;

  while(1) {
    receive(&td, &time, sizeof(time));
    reply(td, NULL, 0);
    delay(time);
    send(td, &time, sizeof(time), NULL, 0);
  }
}

static void until_courier() {
  int td;
  int time;

  while(1) {
    receive(&td, &time, sizeof(time));
    reply(td, NULL, 0);
    delay_until(time);
    send(td, &time, sizeof(time), NULL, 0);
  }
}

/* API 

==================================================================================*/

int delay_until(int time) {
  assert(init == 1, "init_clock_server() must be called exactly once");

  int rc;
  struct clsv_msg msg;
  msg.op = CLOCK_SERVER_DELAY_UNTIL_REQUEST;
  msg.delay = time;
  rc = send(ctd, (char*)&msg, sizeof(msg), NULL, 0);

  return (rc < 0) ? rc : 0;
}

int delay(int ticks) {
  assert(init == 1, "init_clock_server() must be called exactly once");
  int rc;
  struct clsv_msg msg;
  msg.op = CLOCK_SERVER_DELAY_REQUEST;
  msg.delay = ticks;

  rc = send(ctd, &msg, sizeof(msg), NULL, 0);
  return (rc < 0) ? rc : 0;
}

int get_time() { 
  assert(init == 1, "init_clock_server() must be called exactly once");
  int rc, time;
  struct clsv_msg msg;
  msg.op = CLOCK_SERVER_GET_TIME_REQUEST;

  rc = send(ctd, (char*)&msg, sizeof(msg), &time, sizeof(time));
  return (rc < 0) ? rc : time;
}

/* Non blocking delay call. Requires the calling function to
    explicitly receive.  Returns the task id of the task which
    responds to the calling task. This is useful for mulitplexing
    between a delay and other messages of interest. It is expected the
    task will reply as soon as it receives the request indicating that
    the interval has passed. The message ultimately delivered contains
    the delay which was originally passed into the async call. */

/* FIXME this implementation doesn't work if the same task makes concurrent delay requests. */

int delay_async(int time) {
  int i;
  int td = my_tid();
  static couriers[MAX_TASKS] = {0};

  if(couriers[0] == 0)
    for (i = 0; i < MAX_TASKS; i++)
      couriers[i] = -1;

  if(couriers[td] == -1) {
    couriers[td] = create(0, courier); //FIXME (magic priority)
  }

  send(couriers[td], &time, sizeof(time), NULL, 0);
  return couriers[td];
}


int delay_until_async(int time) {
  int i;
  int td = my_tid();
  static couriers[MAX_TASKS] = {0};

  if(couriers[0] == 0)
    for (i = 0; i < MAX_TASKS; i++)
      couriers[i] = -1;

  if(couriers[td] == -1) {
    couriers[td] = create(0, until_courier); //FIXME (magic priority)
  }

  send(couriers[td], &time, sizeof(time), NULL, 0);
  return couriers[td];
}
