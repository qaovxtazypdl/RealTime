#ifndef _IPC_H_
#define _IPC_H_
#include <task.h>

void init_ipc();
int ipc_reply(task_t *src, task_t *dst);
int ipc_send(task_t *src, task_t *dst);
int ipc_receive(task_t *task);

#endif
