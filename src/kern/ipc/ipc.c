#include <common/ds/queue.h>
#include <common/mem.h>
#include <common/math.h>
#include <common/err.h>
#include <ipc.h>
#include <task.h>
#include <kern.h>
#include <sched.h>

#define SEND_QUEUE_SZ 10 /* Maximum number of messages a single receive queue can hold */

/* Received queues index by task id, each queue consists of pointers to tasks 
   which are currently trying to send a message to the task in question. */
static queue_t receive_queues[MAX_TASKS]; 
static task_t* receive_queue_mem[MAX_TASKS][SEND_QUEUE_SZ]; /* Memory for the receive queues. */

void init_ipc() {
  int i;
  void *start;

  /* Initialize the receive queues */
  for (i = 0; i < MAX_TASKS; i++) {
    start = (void*)receive_queue_mem[i];
    queue_init(&receive_queues[i], 
        start, 
        sizeof(receive_queue_mem[0]), 
        sizeof(receive_queue_mem[0][0]));
  }
}

int ipc_reply(task_t *src, task_t *dst) {
  dbg("src: %d, dst: %d, length: %d", src->td, dst->td, src->cpu_state->r2);
  kassert(src && dst, "src and dst must be non-null src td: %x, dst td: %x", src ? src->td : -1, dst ? dst->td : -1);

  const int msg_len = src->cpu_state->r2;
  const int buf_len = dst->cpu_state->ip;
  const int len = MIN(msg_len, buf_len);

  char *buf = (char*)dst->cpu_state->r3;
  char const *msg = (char*)src->cpu_state->r1;

  if(dst->state != TASK_STATE_SEND_BLOCKED) {
    RETURN(src, ERR_TARGET_NOT_SEND_BLOCKED);
    return ERR_TARGET_NOT_SEND_BLOCKED;
  }

  memcpy(buf, msg, len);

  RETURN(src, len);
  RETURN(dst, msg_len);

  return 0;
}

 int ipc_send(task_t *src, task_t *dst) {
  char *buf = dst->cpu_state->r1;
  char const *msg = src->cpu_state->r1;

  const int msg_len = src->cpu_state->r2;
  const int buf_len = dst->cpu_state->r2;
  const int len = MIN(msg_len, buf_len);

  
  int full;

  src->state = TASK_STATE_SEND_BLOCKED;

  if(dst->state == TASK_STATE_RECEIVE_BLOCKED) {
    memcpy(buf, msg, len);
    *((int*)dst->cpu_state->r0) = src->td;
    RETURN(dst, msg_len);
  } else {
    full = queue_add(&receive_queues[dst->td], &src);
    kassert(!full, "Encountered a full receive queue at %d!", dst->td);
  }

  return 0;
}

int ipc_receive(task_t *task) {
  char *buf = (char*)task->cpu_state->r1;
  const int buf_len = task->cpu_state->r2;
  int *src_td = (int*)task->cpu_state->r0;

  task_t *src;
  char *msg;
  int msg_len;

  if(!queue_consume(&receive_queues[task->td], &src)) {
      msg = (char*)src->cpu_state->r1;
      msg_len = src->cpu_state->r2;

      memcpy(buf, msg, MIN(msg_len, buf_len));
      if(src_td)
        *src_td = src->td;
      RETURN(task, msg_len);
      return 0;
  } 

  task->state = TASK_STATE_RECEIVE_BLOCKED;
  return 0;
}
