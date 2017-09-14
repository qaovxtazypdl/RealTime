#include "kernel/syscall_handlers.h"
#include "kernel/constants.h"
#include "kernel/status.h"
#include "kernel/context_switch.h"
#include "common/algs.h"

int syscall_await_event(
  struct TaskDescriptor tds[],
  struct Scheduler *scheduler,
  struct AwaitedEvents *awaited_events,
  int eventid
) {
  if (eventid < 0 || eventid >= NUM_IRQ_MAX) {
    return -1;
  } else {
    struct TaskDescriptor *awaiting_event = scheduler->active;
    scheduler_remove_active_from_queue(scheduler);
    awaiting_event->status = TASK_STATE_AWAIT_BLOCKED;

    if (awaited_events->priority_list_heads[eventid] == NULL) {
      awaited_events->priority_list_heads[eventid] = awaiting_event;
      awaited_events->priority_list_tails[eventid] = awaiting_event;
    } else {
      awaited_events->priority_list_tails[eventid]->next = awaiting_event;
      awaited_events->priority_list_tails[eventid] = awaiting_event;
    }
    return 0;
  }
}

int syscall_send(
  struct TaskDescriptor tds[],
  struct TaskAllocationData *alloc_data,
  struct Scheduler *scheduler,
  struct ByteBuffer sendQueues[],
  struct MessageRequest messageRequests[],
  int tid, char *msg, int msglen, char *reply, int rplen
) {
  struct TaskDescriptor *active = scheduler->active;
  struct TaskDescriptor *receiver = tds + tid;

  struct MessageRequest *senderRequest = messageRequests + active->tid;
  senderRequest->msg = msg;
  senderRequest->msglen = msglen;
  senderRequest->reply = reply;
  senderRequest->rplen = rplen;

  if (tid < 0 || tid >= alloc_data->num_tasks_allocated || receiver->status == TASK_STATE_ZOMBIE) {
    return -2;
  } if (receiver->status == TASK_STATE_SEND_BLOCKED) {
    // update current send request
    int receiver_return_val = 0;
    struct MessageRequest *receiverRequest = messageRequests + receiver->tid;

    // receiver is send blocked.
    active->status = TASK_STATE_REPLY_BLOCKED;
    // directly copy into reply request
    receiver_return_val = memcpy(
      senderRequest->msg, senderRequest->msglen,
      receiverRequest->msg, receiverRequest->msglen
    );
    *(receiverRequest->tid) = active->tid;

    // sets return value of the receive syscall
    if (receiver_return_val < senderRequest->msglen) {
      receiver_return_val = -1;
    }
    receiver->return_val = receiver_return_val;

    // put receiver on the ready queues
    scheduler_add_td(scheduler, receiver);
  } else {
    // not send blocked - add active task id to the queue
    if (!is_byte_buffer_full(sendQueues + receiver->tid)) {
      byte_buffer_push(sendQueues + receiver->tid, (char) active->tid);
    } else {
      return -3;
    }

    // set sender to receive block.
    active->status = TASK_STATE_RECEIVE_BLOCKED;
  }
  scheduler_remove_active_from_queue(scheduler);

  return 0;
}

void syscall_receive(
  struct TaskDescriptor tds[],
  struct Scheduler *scheduler,
  struct ByteBuffer sendQueues[],
  struct MessageRequest messageRequests[],
  int *tid, char *msg, int msglen
) {
  struct TaskDescriptor *receiver = scheduler->active;

  struct MessageRequest *receiverRequest = messageRequests + receiver->tid;
  receiverRequest->tid = tid;
  receiverRequest->msg = msg;
  receiverRequest->msglen = msglen;

  if (is_byte_buffer_empty(sendQueues + receiver->tid)) {
    // if no send queue items, we send-block.
    receiver->status = TASK_STATE_SEND_BLOCKED;
    scheduler_remove_active_from_queue(scheduler);
  } else {
    // take action if there are stuff on the queue.
    int sender_tid = byte_buffer_pop(sendQueues + receiver->tid);
    struct TaskDescriptor *sender = tds + sender_tid;
    struct MessageRequest *senderRequest = messageRequests + sender_tid;
    int receiver_return_val = 0;

    // directly copy into reply request
    receiver_return_val = memcpy(
      senderRequest->msg, senderRequest->msglen,
      receiverRequest->msg, receiverRequest->msglen
    );
    *(receiverRequest->tid) = sender_tid;

    // reply-block sender
    sender->status = TASK_STATE_REPLY_BLOCKED;

    // populate the return value
    if (receiver_return_val < senderRequest->msglen) {
      receiver_return_val = -1;
    }
    receiver->return_val = receiver_return_val;

    // make receiver ready again
    scheduler_move_active_to_ready(scheduler);
  }
}

int syscall_reply(
  struct TaskDescriptor tds[],
  struct TaskAllocationData *alloc_data,
  struct Scheduler *scheduler,
  struct ByteBuffer sendQueues[],
  struct MessageRequest messageRequests[],
  int tid, char *reply, int rplen
) {
  struct TaskDescriptor *sender = tds + tid;

  if (tid < 0 || tid >= alloc_data->num_tasks_allocated || sender->status == TASK_STATE_ZOMBIE) {
    return -2;
  } else if (sender->status == TASK_STATE_REPLY_BLOCKED) {
    struct MessageRequest *senderRequest = messageRequests + tid;
    int return_val = 0;

    // directly copy into reply request
    return_val = memcpy(
      reply, rplen,
      senderRequest->reply, senderRequest->rplen
    );

    // check for truncation
    if (return_val < rplen) {
      return_val = -1;
    }

    // set return value of sender
    sender->return_val = return_val;

    // make both sender and receiver ready again.
    scheduler_move_active_to_ready(scheduler);
    scheduler_add_td(scheduler, sender);

    return return_val;
  } else {
    sender->return_val = -3;
    return -3;
  }
}

int syscall_create(
  struct TaskDescriptor tds[],
  struct TaskAllocationData *alloc_data,
  struct Scheduler *scheduler,
  int parent_tid,
  int priority,
  void (*entryPoint)()
) {
  if (priority < 0 || priority > MAX_PRIORITY) {
    // invalid priority
    return -1;
  }

  if (alloc_data->num_tasks_allocated >= NUM_TASKS_MAX) {
    // no more task descriptors
    return -2;
  }

  int tid = alloc_data->num_tasks_allocated;
  struct TaskDescriptor *td = tds + tid;

  // initialize task's stack to unroll
  td->tid = tid;
  td->sp = (int *) alloc_data->next_stack_pointer;
  td->spsr = 0x10; // initialize user mode, flag bits off initially
  td->pc = (int *) ((int)entryPoint + 0x00218000);
  td->return_val = -100;
  td->priority = priority;
  td->parent_tid = parent_tid;
  td->next = NULL;

  // put relevant shit on the stack
  // push lr
  td->sp--;
  *(td->sp) = (int)&kernel_entry + 0x00218000;

  // push r0-r12
  int register_init_it;
  for (register_init_it = 12; register_init_it >= 0; register_init_it--) {
    td->sp--;
    *(td->sp) = 0x0;
  }

  alloc_data->next_stack_pointer = alloc_data->next_stack_pointer - TASK_STACK_SIZE;
  alloc_data->num_tasks_allocated++;

  scheduler_add_td(scheduler, td);

  return tid;
}

