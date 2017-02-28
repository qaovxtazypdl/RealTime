#ifndef _EVENT_MANAGER_H_
#define _EVENT_MANAGER_H_
#include <task.h>
#include <common/event.h>

void init_event();
int event_subscribe(struct task *task, enum event type);
int event_notify(enum event type, int retval, void *payload, int payload_sz); 
int event_blocked_task_count();

#endif
