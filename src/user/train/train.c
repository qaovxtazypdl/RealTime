#include <train.h>
#include <io.h>
#include <ns.h>
#include <clock_server.h>
/* All interactions with the train should be done using functions in this file */

#define MAX_TRAINS 100

static int speeds[MAX_TRAINS]; /* FIXME */

void tr_set_switch(int turnout, int curved) { 
  putc(TRAIN_COM, curved ? 34 : 33 ); 
  putc(TRAIN_COM, (char)turnout); 
  putc(TRAIN_COM, 32); 
}

void tr_request_sensor_data() { putc(TRAIN_COM, 133); }

void tr_set_speed(int train, int speed) {
  speeds[train] = speed;
  putc(TRAIN_COM, (char)speed);
  putc(TRAIN_COM, (char)train);
}

void tr_reverse(int train) { 
  int speed = speeds[train];

  putc(TRAIN_COM, 0);
  putc(TRAIN_COM, (char)train);

  delay(300);

  putc(TRAIN_COM, 15);
  putc(TRAIN_COM, (char)train);

  putc(TRAIN_COM, (char)speed);
  putc(TRAIN_COM, (char)train);
}
