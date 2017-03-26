#include <common/syscall.h>
#include <io.h>
#include <gen/cmd.h>
#include <train.h>
#include <console.h>

static void inline handle_io() {
	char c;
  char input_buffer[INPUT_BUFFER_SZ];
  int input_sz = 0;

  printf(COM2, "%m%s", (int[]){ 0, PROMPT_LINE }, PROMPT);
  while(!getc(COM2, &c)) {
    switch(c) {
      case '\r':
      case '\n':
        printf(COM2, "%s\r\n\r\n", CLEAR_DOWN);
        input_buffer[input_sz] = '\0';

        if(handle_cmd(input_buffer))
          putstr(COM2, "Error, invalid command! Type 'help' for a list of valid commands.\r\n");

        input_buffer[0] = '\0';
        input_sz = 0;

        printf(COM2, "%m%s%s",
            (int[]){ 0, PROMPT_LINE },
            CLEAR_LINE,
            PROMPT);
        break;
      case '\b':
        if(input_sz) {
          if(input_sz) {
            printf(COM2, "\b%s", CLEAR_EOL);
            input_buffer[--input_sz] = '\0';
          }
        }
        break;
      default:
        if(input_sz < INPUT_BUFFER_SZ) {
          putc(COM2, c);
          input_buffer[input_sz++] = c;
        }
    }
  }
}


static void inline initialize_switch_table() {
	int i;
	char table[1024];
	char *tail = table;

	tail += sprintf(tail, "%s%m", CLEAR_LINE, (int[]){ 0, SWITCH_TABLE_LINE_NUM });

	for (i = 0; i < 18; i++) {
    tail += sprintf(tail, "\x1B[32m%s%d\tC\x1B[39m\n\r", CLEAR_LINE, i + 1);
		tr_set_switch(i + 1, 1);
	}

	for (i = 0; i < 4; i++) {
    tail += sprintf(tail, "\x1B[32m%s%d\tC\x1B[39m\n\r", CLEAR_LINE, i + 153);
		tr_set_switch(i + 153, 1);
	}

	putstr(COM2, table);
}

void console_task() {
	printf(COM2, "%s%m", CLEAR_SCREEN, (int[]){0,0});
  initialize_switch_table();
  handle_io();
}

void update_switch_entry(int turnout, int curved) {
  int line;

  if(turnout > 152)
    line = SWITCH_TABLE_LINE_NUM + 18 + turnout - 153;
  else
    line = SWITCH_TABLE_LINE_NUM + turnout - 1;

  printf(COM2,
      "%s%s%m%d\t%c%s%s",
      curved ? "\x1B[32m" : "\x1B[36m",
      SAVE_CURSOR,
      (int[]){0, line},
      turnout,
      curved ? 'C' : 'S',
      "\x1B[39m",
      RESTORE_CURSOR);
}

void update_train_info(
  int num,
  int line,
  enum direction reverse,
  struct position *position,
  int velocity,
  int acceleration
) {
  printf(COM2,
    "%s%m%sTRAIN{%d}:\t%dmm past node %s, [%s], v=%d, a=%d%s",
    SAVE_CURSOR,
    /* Avoid magic numbers, define this as a macro and move this function into console.c */
    (int[]){0, TRAIN_INFO_LINE_START + line},
    CLEAR_LINE,
    num,
    position->offset,
    (position == NULL || position->node == NULL) ? "X" : position->node->name,
    reverse == FORWARD ? "FWD" : "REV",
    velocity,
    acceleration,
    RESTORE_CURSOR
  );
}

void print_broken(char *name, int line) {
  printf(COM2,
    "%s%m%sBROKEN: %s\n\r%s",
    SAVE_CURSOR,
    (int[]){0, SENSOR_INFO_LINE_START + line},
    CLEAR_LINE,
    name,
    RESTORE_CURSOR
  );
}

void update_sensor_display(struct movement_state *state, int delta_t, int delta_d, int line) {
  char *accel_state;
  if (state->accel_from_last_sensor == ACCELERATING) {
    accel_state = "ACCEL";
  } else if (state->accel_from_last_sensor == DECELERATING) {
    accel_state = "DECEL";
  } else {
    accel_state = "";
  }

  int current_time = get_time() % 10000;
  printf(COM2,
    "%s%m%sTRAIN{%d}\tt: %d\tsens: %s\tdel_t: %d\tdel_d: %d mm\texp_next: %s\t%s%s",
    SAVE_CURSOR,
    (int[]){0, SENSOR_INFO_LINE_START + line},
    CLEAR_LINE,
    state->train_num,
    current_time,
    state->position.node ? state->position.node->name : "Y",
    delta_t, delta_d,
    state->expected_next_sensor ? state->expected_next_sensor->name : "X",
    accel_state,
    RESTORE_CURSOR
  );
}
