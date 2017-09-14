#ifndef _KERNEL__SYSCALL_H_
#define _KERNEL__SYSCALL_H_

// all syscall IDs are 8 bit
// must change in both this file and the syscall.s file
#define SYSCALL_CREATE_ID 0x1
#define SYSCALL_MY_TID_ID 0x10
#define SYSCALL_MY_PARENT_TID_ID 0x11

#define SYSCALL_MY_PROFILER_TIME 0xf0
#define SYSCALL_PASS_ID 0xfe
#define SYSCALL_EXIT_ID 0xff
#define SYSCALL_SHUTDOWN_ID 0xfc

#define SYSCALL_SEND_ID 0x20
#define SYSCALL_RECEIVE_ID 0x21
#define SYSCALL_REPLY_ID 0x22

#define SYSCALL_AWAIT_EVENT_ID 0x30


int Create(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();


int Send(int tid, char *msg, int msglen, char *reply, int rplen);
int Receive(int *tid, char *msg, int msglen);
int Reply(int tid, char *reply, int rplen);

int AwaitEvent(int eventid);

void Shutdown();
int MyProfilerTime(int *my_time, int *total_time);


#endif
