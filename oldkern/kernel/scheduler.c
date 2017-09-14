#include "kernel/scheduler.h"
#include "kernel/kernel_ds.h"
#include "kernel/status.h"
#include "kernel/constants.h"

void scheduler_init(struct Scheduler *scheduler) {
  int iterator;
  for (iterator = 0; iterator < NUM_SCHEDULER_PRIORITIES; iterator++) {
    scheduler->priority_list_heads[iterator] = NULL;
    scheduler->priority_list_tails[iterator] = NULL;
  }

  scheduler->flag_shutdown = FALSE;

  scheduler->clz_lookup_map[0] = 0;
  scheduler->clz_lookup_map[1] = 31;
  scheduler->clz_lookup_map[2] = 9;
  scheduler->clz_lookup_map[3] = 30;
  scheduler->clz_lookup_map[4] = 3;
  scheduler->clz_lookup_map[5] = 8;
  scheduler->clz_lookup_map[6] = 13;
  scheduler->clz_lookup_map[7] = 29;
  scheduler->clz_lookup_map[8] = 2;
  scheduler->clz_lookup_map[9] = 5;
  scheduler->clz_lookup_map[10] = 7;
  scheduler->clz_lookup_map[11] = 21;
  scheduler->clz_lookup_map[12] = 12;
  scheduler->clz_lookup_map[13] = 24;
  scheduler->clz_lookup_map[14] = 28;
  scheduler->clz_lookup_map[15] = 19;
  scheduler->clz_lookup_map[16] = 1;
  scheduler->clz_lookup_map[17] = 10;
  scheduler->clz_lookup_map[18] = 4;
  scheduler->clz_lookup_map[19] = 14;
  scheduler->clz_lookup_map[20] = 6;
  scheduler->clz_lookup_map[21] = 22;
  scheduler->clz_lookup_map[22] = 25;
  scheduler->clz_lookup_map[23] = 20;
  scheduler->clz_lookup_map[24] = 11;
  scheduler->clz_lookup_map[25] = 15;
  scheduler->clz_lookup_map[26] = 23;
  scheduler->clz_lookup_map[27] = 26;
  scheduler->clz_lookup_map[28] = 16;
  scheduler->clz_lookup_map[29] = 27;
  scheduler->clz_lookup_map[30] = 17;
  scheduler->clz_lookup_map[31] = 18;

  scheduler->nonempty_queue_bitmask = 0;
}

void scheduler_add_td(struct Scheduler *scheduler, struct TaskDescriptor *td) {
  int priority = td->priority;

  td->next = NULL;
  if (scheduler->priority_list_heads[priority] == NULL) {
    scheduler->nonempty_queue_bitmask |= (1 << (MAX_PRIORITY - priority));
    scheduler->priority_list_heads[priority] = td;
    scheduler->priority_list_tails[priority] = td;
  } else {
    scheduler->priority_list_tails[priority]->next = td;
    scheduler->priority_list_tails[priority] = td;
  }
  td->status = TASK_STATE_READY;
}

void scheduler_remove_active_from_queue(struct Scheduler *scheduler) {
  struct TaskDescriptor *originalHead;
  int priority = scheduler->active->priority;

  originalHead = scheduler->priority_list_heads[priority];
  scheduler->priority_list_heads[priority] = scheduler->priority_list_heads[priority]->next;
  originalHead->next = NULL;
  if (scheduler->priority_list_heads[priority] == NULL) {
    scheduler->priority_list_tails[priority] = NULL;
    scheduler->nonempty_queue_bitmask &= ~(1 << (MAX_PRIORITY - priority));
  }
}

void scheduler_move_active_to_ready(struct Scheduler *scheduler) {
  struct TaskDescriptor *originalHead;
  int priority = scheduler->active->priority;

  originalHead = scheduler->priority_list_heads[priority];
  scheduler->priority_list_tails[priority]->next = scheduler->priority_list_heads[priority];
  scheduler->priority_list_heads[priority] = scheduler->priority_list_heads[priority]->next;
  scheduler->priority_list_tails[priority] = originalHead;
  scheduler->priority_list_tails[priority]->next = NULL;
}
