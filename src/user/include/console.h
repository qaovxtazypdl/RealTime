#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <movement.h>

#define SWITCH_TABLE_LINE_NUM 5
#define PROMPT_LINE (SWITCH_TABLE_LINE_NUM + 23)
#define TRAIN_INFO_LINE_START (PROMPT_LINE + 14)
#define SENSOR_INFO_LINE_START (TRAIN_INFO_LINE_START + 4)
#define INPUT_BUFFER_SZ 200
#define PROMPT "> "

void console_task();
void update_switch_entry(int turnout, int curved);
void update_train_info(
  int num,
  int line,
  enum direction reverse,
  struct position *position,
  int velocity,
  int acceleration
);

void print_broken(char *name, int line);
void update_sensor_display(struct movement_state *state, int delta_t, int delta_d, int line);


#endif
