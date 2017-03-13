#ifndef _CLOCK_SERVER_H_
#define _CLOCK_SERVER_H_

void init_clock_server();
int delay_async(int time);
int delay_until(int time);
int delay_until_async(int time);
int delay(int ticks);
int get_time();

#endif
