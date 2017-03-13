#include<ts7200.h>
#include<timer.h>

/* Set/enable TIMER3 in pre-load mode with the given val */
static void timer_load(int val) {
  int *timer_load = (int *)TIMER3_BASE;
  int *ctl = (int *) (TIMER3_BASE + CRTL_OFFSET);

  /* Initialize timer */
  *ctl |= MODE_MASK; /* Pre-load running mode */
  *ctl |= CLKSEL_MASK; /* 508 KHZ */
  *ctl |= ENABLE_MASK; /* Enable bit :S */

  *timer_load = val;
}

/* Return the value in the timer value register */
int timer_val() {
  return *((int *)(TIMER3_BASE + VAL_OFFSET));
}

void initialize_timer(int tick_size) {
  timer_load(TIMER3_CLK_RATE * tick_size);
}

void timer_disable() {
  int *ctl = (int *) (TIMER3_BASE + CRTL_OFFSET);
  *ctl &= ~ENABLE_MASK; /* Enable bit :S */
  ctl = (int *) (TIMER3_BASE + CRTL_OFFSET);
  *ctl &= ~ENABLE_MASK; /* Enable bit :S */
}

void timer_clear_interrupt() {
  *((int*)(TIMER3_BASE + CLR_OFFSET)) = 1; 
#ifdef EMULATOR_BUILD
  //FIXME this shouldn't be necessary with one-shot off
  timer_load(10 * TIMER3_CLK_RATE);
#endif
}

int timer4_start() {
#ifndef EMULATOR_BUILD //FIXME
    int volatile *low = (int*)0x80810060;
    int volatile *high = (int*)0x80810064;

    /* Reset/enable timer */
    *high &= ~0x100;
    *high |= 0x100;
#endif

    return 0;
}

int timer4_val() {
  #ifndef EMULATOR_BUILD //FIXME
      int volatile *low = (int*)0x80810060;
      int volatile *high = (int*)0x80810064;
  
      return *low;
  #else
      return 0;
  #endif
}
