#include "kernel/syscall.h"
#include "bwio/bwio.h"
#include "tasks/name_server.h"


void nst_test_task_1() {
  int retval = -55;

  retval = MyTid();
  bwprintf(COM2, "UM - mytid1 - %d\n\r", retval);
  Pass();

  retval = RegisterAs("task1");
  bwprintf(COM2, "UM - 1 - %d\n\r", retval);
  Pass();

  retval = RegisterAs("task1alt");
  bwprintf(COM2, "UM - 1 - %d\n\r", retval);
  Pass();

  retval = WhoIs("task2");
  bwprintf(COM2, "UM - 1 - %d\n\r", retval);
  Pass();

  retval = WhoIs("task2alt");
  bwprintf(COM2, "UM - 1 - %d\n\r", retval);
  Pass();

  retval = WhoIs("task2shite");
  bwprintf(COM2, "UM - 1 - %d\n\r", retval);
  Exit();
}

void nst_test_task_2() {
  int retval = -55;

  retval = MyTid();
  bwprintf(COM2, "UM - mytid2 - %d\n\r", retval);
  Pass();

  retval = RegisterAs("task2");
  bwprintf(COM2, "UM - 2 - %d\n\r", retval);
  Pass();

  retval = RegisterAs("task2alt");
  bwprintf(COM2, "UM - 2 - %d\n\r", retval);
  Pass();

  retval = WhoIs("task1");
  bwprintf(COM2, "UM - 2 - %d\n\r", retval);
  Pass();

  retval = WhoIs("task1alt");
  bwprintf(COM2, "UM - 2 - %d\n\r", retval);
  Pass();

  retval = WhoIs("task1shite");
  bwprintf(COM2, "UM - 2 - %d\n\r", retval);
  Exit();
}

void nst_test_bootstrap() {
  int retval;
  bwprintf(COM2, "FUT - Starting!\n\r");

  retval = Create(22, &nst_test_task_1);
  bwprintf(COM2, "FUT - test task id %d\n\r", retval);

  retval = Create(22, &nst_test_task_2);
  bwprintf(COM2, "FUT - test task id %d\n\r", retval);
  Exit();
}
