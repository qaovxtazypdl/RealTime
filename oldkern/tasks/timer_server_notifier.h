#ifndef _TASKS__TIMER_SERVER_NOTIFIER_H_
#define _TASKS__TIMER_SERVER_NOTIFIER_H_

#define TIMERSERVER_DELAY 0
#define TIMERSERVER_TIME 1
#define TIMERSERVER_DELAYUNTIL 2
#define TIMERSERVER_NOFITYTICK 3

struct TimerServerRequest {
  char type;
  int ticks;
};

void task__timer_server();
void task__timer_notifier();

// reply scheme -> -1 if fail, >-1 if success

/*
0 Success.
-1 The clock server task id is invalid.
-2 The delay was zero or negative.
*/
int Delay(int tid, int ticks);

/*
>-1 The time in ticks since the clock server initialized.
-1 The clock server task id is invalid.
*/
int Time(int tid);

/*
0 Success.
-1 The clock server task id is invalid.
-2 The delay was zero or negative.
*/
int DelayUntil(int tid, int ticks);

#endif
