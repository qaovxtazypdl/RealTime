#include "kernel/syscall.h"
#include "tasks/first.h"
#include "tasks/idle.h"
#include "common/assert.h"
#include "tasks/name_server.h"
#include "tasks/timer_server_notifier.h"
#include "tasks/uart_server_notifier.h"
#include "tasks/io_controller.h"
#include "bwio/bwio.h"
#include "emul/ts7200.h"

// debug
#include "tasks/test/srrtest.c"
#include "tasks/test/stttimed.c"
#include "tasks/test/rps.c"
#include "tasks/test/sw_irq.c"
#include "tasks/test/name_server_test.c"
#include "tasks/test/clockserver_test.c"

/*
void task__first_test_sendrecrepl() {
  Create(17, &simple_syscall_test);
  Exit();
}
*/

/*
void task__first_test_ns() {
  Create(0, &task__name_server);
  Create(17, &nst_test_bootstrap);
  Exit();
}

void task__first_sw_irq() {
  Create(2, &task__name_server);
  Create(0, &server_creator);
  Create(5, &notifier_creator);
  Create(13, &task__test_sw_irq);
  Exit();
}
*/

/*
//void task__first() {
void task__first_test_timed_sendrecrepl() {
  Create(2, &task__name_server);

  // init servers before notifiers
  // servers should be prio 3
  // notifiers should be prio 2
  // nameserver prio 1
  Create(0, &server_creator);
  Create(5, &notifier_creator);

  Create(17, &simple_timed_syscall_test);

  Create(31, &task__idle);
  Exit();
}
*/


void notifier_creator() {
  Create(2, &task__timer_notifier);

  Create(2, &task__uart1_ready_notifier);
  Create(2, &task__uart1_modem_notifier);
  Create(2, &task__uart1_transmit_notifier);
  Create(2, &task__uart1_receive_notifier);

  Create(2, &task__uart2_transmit_notifier);
  Create(2, &task__uart2_receive_notifier);

  Exit();
}

void server_creator() {
  Create(5, &task__timer_server);

  // must init transmit before receive, as receive needs NS lookup
  int tid = -1;
  int retval;
  int ensure_tid = -1;
  char reply;

  ensure_tid = Create(5, &task__uart_transmit_server);
  retval = Receive(&tid, NULL, 0);
  if (retval < 0) {
    bwprintf(COM2, "FUT - server 1 receive failed");
  }
  ASSERT(tid == ensure_tid, "Server init TID reply FUT doesnt match.");
  reply = COM1;
  retval = Reply(tid, (char *) &reply, 1);
  if (retval < 0) {
    bwprintf(COM2, "FUT - server 1 reply failed");
  }

  ensure_tid = Create(5, &task__uart_transmit_server);
  retval = Receive(&tid, NULL, 0);
  if (retval < 0) {
    bwprintf(COM2, "FUT - server 2 receive failed");
  }
  ASSERT(tid == ensure_tid, "Server init TID reply FUT doesnt match.");
  reply = COM2;
  retval = Reply(tid, (char *) &reply, 1);
  if (retval < 0) {
    bwprintf(COM2, "FUT - server 2 reply failed");
  }

  ensure_tid = Create(5, &task__uart_receive_server);
  retval = Receive(&tid, NULL, 0);
  if (retval < 0) {
    bwprintf(COM2, "FUT - server 3 receive failed");
  }
  ASSERT(tid == ensure_tid, "Server init TID reply FUT doesnt match.");
  reply = COM1;
  retval = Reply(tid, (char *) &reply, 1);
  if (retval < 0) {
    bwprintf(COM2, "FUT - server 3 reply failed");
  }

  ensure_tid = Create(5, &task__uart_receive_server);
  retval = Receive(&tid, NULL, 0);
  if (retval < 0) {
    bwprintf(COM2, "FUT - server 4 receive failed");
  }
  ASSERT(tid == ensure_tid, "Server init TID reply FUT doesnt match.");
  reply = COM2;
  retval = Reply(tid, (char *) &reply, 1);
  if (retval < 0) {
    bwprintf(COM2, "FUT - server 4 reply failed");
  }

  Exit();
}

void controller_creator() {
  int tid = -1;
  int retval;
  int ensure_tid = -1;

  Create(8, &task__timer_display);


  ensure_tid = Create(8, &task__track_init);
  // wait for switch init
  retval = Receive(&tid, NULL, 0);
  if (retval < 0) {
    bwprintf(COM2, "FUT - controller 1 receive failed");
  }
  ASSERT(tid == ensure_tid, "Server init TID reply FUT doesnt match.");
  retval = Reply(tid, NULL, 0);
  if (retval < 0) {
    bwprintf(COM2, "FUT - controller 1 reply failed");
  }


  // switch, sensor, trian controler
  Create(9, &task__switch_controller);
  Create(9, &task__sensor_controller);
  Create(9, &task__train_controller);

  // command parser
  Create(9, &task__command_parser);

  Exit();
}

//void original_task__first() {
void task__first() {
  //this always needs to be the first create, to force the name server to have a tid of 1.
  Create(1, &task__name_server);

  // init servers before notifiers
  // controllers (io managers) should be prio 8 9 10
  // servers should be prio 5 6 7
  // notifiers should be prio 2 3 4
  // nameserver prio 1
  Create(5, &server_creator);
  Create(8, &notifier_creator);

  // open up uart interrupts
  Create(6, &task__uart_initializer);

  // initialize controllers
  Create(11, &controller_creator);

  // idle task at lowest prio 31
  Create(31, &task__idle);

  Exit();
}

