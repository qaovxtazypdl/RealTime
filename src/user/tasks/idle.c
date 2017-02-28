#include <common/syscall.h>
#include <common/bwio.h>
#include <common/timer.h>
#include <ns.h>
#include <idle_task.h>
#include <clock_server.h>

static int idle_ticks = 0;
#define IDLE_LINE_NUM 1

void idle_print() {
  int ticks;
  while(1) {
    delay(IDLE_MEASUREMENT_INTERVAL / 10);
    ticks = idle_ticks;
    //FIXME use implemented IO
    //printf("%m%d%%", (int[]) { 0, IDLE_LINE_NUM }, (100 * ticks) / (983 * IDLE_MEASUREMENT_INTERVAL)); 
    idle_ticks = 0;
  }
}

void idle() {
  int last_time, ctime;
  timer4_start();
  while(1){
    ctime = timer4_val();
    if((ctime - last_time) == 1)
      idle_ticks++;

    last_time = ctime;
  }
}
