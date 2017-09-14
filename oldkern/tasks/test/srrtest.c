#include "kernel/syscall.h"
#include "bwio/bwio.h"


void task__producer() {
  bwprintf(COM2, "UM - Producer initialized\n\r");

  int tid = -1;
  int rcvlen = -11;
  int rplsts = -11;
  char msgbuf[6], replybuf[6];

  bwprintf(COM2, "UM - Producer - receiving...\n\r");
  rcvlen = Receive(&tid, msgbuf, 6);
  bwprintf(COM2, "UM - Producer - request received from tid: %d, len: %d - %x %x %x %x\n\r", tid, rcvlen, msgbuf[0], msgbuf[1], msgbuf[2], msgbuf[3]);

  replybuf[0] = 0x12;
  replybuf[1] = 0x23;
  replybuf[2] = 0x34;
  replybuf[3] = 0x45;
  rplsts = Reply(tid, replybuf, 4);
  bwprintf(COM2, "UM - Producer - reply status: %d\n\r", rplsts);

  bwprintf(COM2, "UM - Producer - Exited\n\r");
  Exit();
}

void task__consumer() {
  bwprintf(COM2, "UM - Consumer initialized\n\r");

  char msgbuf[6], replybuf[6];
  int rcvlen = 0;

  msgbuf[0] = 0xab;
  msgbuf[1] = 0xbc;
  msgbuf[2] = 0xcd;
  msgbuf[3] = 0xde;
  bwprintf(COM2, "UM - Consumer - request sending - %x %x %x %x\n\r", msgbuf[0], msgbuf[1], msgbuf[2], msgbuf[3]);
  rcvlen = Send(2, msgbuf, 4, replybuf, 6);
  bwprintf(COM2, "UM - Consumer - message received: len %d - %x %x %x %x\n\r", rcvlen, replybuf[0], replybuf[1], replybuf[2], replybuf[3]);

  bwprintf(COM2, "UM - Consumer - Exited\n\r");
  Exit();
}

void simple_syscall_test() {
  int retval;
  bwprintf(COM2, "FUT - Starting!\n\r");

  retval = Create(21, &task__producer);
  bwprintf(COM2, "FUT - Producer id %d\n\r", retval);

  retval = Create(22, &task__consumer);
  bwprintf(COM2, "FUT - Consumer id %d\n\r", retval);

  Exit();
}
