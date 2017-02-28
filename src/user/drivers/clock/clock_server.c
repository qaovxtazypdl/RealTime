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
