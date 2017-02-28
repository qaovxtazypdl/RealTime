#include <common/syscall.h>
#include <common/event.h>
#include <assert.h>

int create(int priority, void (*start)()) {
  int result;

  SYS_POPULATE_ARGS(priority, start);
  SYSCALL(SYSCALL_CREATE);
  SYS_STORE_RESULT(result);

  return result;
}


int my_tid() {
  int result;
  SYSCALL(SYSCALL_MYTID);
  SYS_STORE_RESULT(result);
  return result;
}

int my_parent_tid() {
  int result;
  SYSCALL(SYSCALL_PARENT_TID);
  SYS_STORE_RESULT(result);
  return result;
}

void pass() {
  SYSCALL(SYSCALL_PASS);
}

void exit() {
  SYSCALL(SYSCALL_EXIT);
}

int await_event(enum event type, void *payload) {
  int result;
  SYS_POPULATE_ARGS(type, payload);
  SYSCALL(SYSCALL_AWAIT_EVENT);
  SYS_STORE_RESULT(result);
  return result;
}

int send(int tid, const void *msg, int msg_sz, void *reply, int repl_sz) {
  int result;

  SYS_POPULATE_ARGS(tid, msg, msg_sz, reply, repl_sz);
  SYSCALL(SYSCALL_SEND);
  SYS_STORE_RESULT(result);

  return result;
}

int receive(int *td, void *buf, int buf_len) {
  int result;

  SYS_POPULATE_ARGS(td, buf, buf_len);
  SYSCALL(SYSCALL_RECEIVE);
  SYS_STORE_RESULT(result);

  return result;
}


int reply(int td, void *msg, int msg_len) {
  int result;

  SYS_POPULATE_ARGS(td, msg, msg_len);
  SYSCALL(SYSCALL_REPLY);
  SYS_STORE_RESULT(result);

  return result;
}
