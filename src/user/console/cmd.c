#include <common/string.h>
#include <common/syscall.h>
#include <gen/cmd.h>
#include <train.h>
#include <io.h>
#include <console.h>

static void update_switch_entry(int turnout, int curved) {
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

void handle_sw_cmd(char *turnout, char *pos) {
  int curved = streq(pos, "C") && streq(pos, "c") ? 0 : 1;
  int sw, i;

  if(!streq(turnout, "*")) {
    printf(COM2, "FLIPPING ALL THE SWITCHES\r\n");
    for (i = 0; i < 18; i++) {
      tr_set_switch(i + 1, curved);
      update_switch_entry(i + 1, curved);
    }
    for (i = 0; i < 4; i++) {
      tr_set_switch(i + 153, curved);
      update_switch_entry(i + 153, curved);
    }
  } else {
    sw = strtoi(turnout, 0, NULL);
    if(!sw) {
      printf(COM2, "Usage: sw <1-18, 153-154> <c | s>\r\n");
      return;
    }

    printf(COM2, "Attempting to flip switch %d\r\n", sw);
    tr_set_switch(sw, curved);
    update_switch_entry(sw, curved);
  }
}

void handle_rv_cmd(int train) {
  printf(COM2, "Attempting to reverse train %d\r\n", train);
  tr_reverse(train);
}

void handle_tr_cmd(int train, int speed) {
  printf(COM2, "Attempting to start train %d at speed %d\r\n", train, speed);
  tr_set_speed(train, speed);
}


void handle_q_cmd() {
  SYSCALL(SYSCALL_TERMINATE); /* FIXME wrap this */
}

void handle_help_cmd() {
  putstr(COM2, "Valid Commands: \r\n\r\n");
  printf(COM2, "\ttr <train number> <speed> - Sets the speed of the train\r\n");
  printf(COM2, "\tsw <switch number> <C | S> - Changes the position of the switch to either curved or straight\r\n");
  printf(COM2, "\trv <train number> - Reverses the given train\r\n");
  printf(COM2, "\tq - Exits the program\r\n");
}
