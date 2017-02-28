#ifndef _TIMER_H_
#define _TIMER_H_

/* Consumes tick size in milliseconds */
void initialize_timer(int tick_size);
int timer_val();
void timer_disable();
void timer_clear_interrupt();
int timer4_start();
int timer4_val();

#endif
