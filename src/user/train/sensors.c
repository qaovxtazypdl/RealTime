#include <train.h>
#include <ns.h>
#include <clock_server.h>
#include <io.h>

#define SENSOR_DATA_INTERVAL 10
#define SENSOR_LINE_NUM 1

static char active_switches[5][16];

static void process_sensor_data(char active[5][16]) {
  char b;
  int i, j, num;

  for (i = 0; i < 10; i++) {
    getc(TRAIN_COM, &b);

    for (j = 0; j < 8; j++) {
      if(i % 2)
        num = 16 - j;
      else
        num = 8 - j;

      if((1 << j) & b) {
        active[i/2][num] = 1;
      }
    }
  }
}


void sensor_printer() {
  int i, j;
  char str[1024];
  int sz = 0, seen = 0;

  delay(50);
  while(1) {
    delay(SENSOR_DATA_INTERVAL);
    tr_request_sensor_data();
    process_sensor_data(active_switches);

    sz = 0;
    sz += sprintf(str, "%s%m%s", SAVE_CURSOR, (int[]){ 0, SENSOR_LINE_NUM }, CLEAR_LINE);
    sz += sprintf(str + sz, "[ ");

    seen = 0;
    for (i = 0; i < sizeof(active_switches)/sizeof(active_switches[0]); i++)
      for (j = 0; j < sizeof(active_switches[0]); j++)
        if(active_switches[i][j]) {
          seen++;
          sz += sprintf(str + sz, "%c%d | ", 'A' + i, j);
          active_switches[i][j] = 0;
        }

    if(seen)
      sz -= 3;
    sz += sprintf(str + sz, " ]%s", RESTORE_CURSOR);
    putstr(COM2, str);
  }
}
/* Periodically sends a NULL terminated list of track_nodes to the calling task corresponding to actie sensors.
Returns the td of the the task which sends. */
int sensor_subscribe() {
}
