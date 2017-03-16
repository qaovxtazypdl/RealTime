#include <common/string.h>
#include <common/syscall.h>
#include <gen/cmd.h>
#include <train.h>
#include <io.h>
#include <track.h>
#include <console.h>

static int trains[MAX_TRAIN] = {0};

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


void handle_set_track_cmd(char *track) {
  if(!streq(track, "A") || !streq(track, "a")) {
    g_is_track_a = 1;
    init_tracka(g_track, g_sensors);
    printf(COM2, "Setting track to A");
  } else {
    g_is_track_a = 0;
    init_trackb(g_track, g_sensors);
    printf(COM2, "Setting track to B");
  }
}

void handle_path_cmd(char *src, char *dst) {
  struct track_node *srcn = lookup_track_node(src);
  struct track_node *dstn = lookup_track_node(dst);
  struct track_node *path[MAX_PATH_LEN];

  if(!dstn || !srcn) {
    printf(COM2, "%s is not a valid node\r\n", src ? dst : src);
    return;
  }

  path_find(srcn, dstn, path);
  path_activate(path);
}

void handle_q_cmd() {
  SYSCALL(SYSCALL_TERMINATE); /* FIXME wrap this */
}

void handle_route_train_cmd(int num, char *src, char *dst) {
  int td = trains[num];
  if(!td) {
    printf(COM2, "Train %d does not exist\r\n", num);
    return;
  }
  int len;
  struct track_node *srcn = lookup_track_node(src);
  struct track_node *dstn = lookup_track_node(dst);
  struct track_node *path[MAX_PATH_LEN];

  if(!dstn || !srcn) {
    printf(COM2, "%s is not a valid node\r\n", src ? dst : src);
    return 0;
  }

  len = path_find(srcn, dstn, path);
  train_set_path(td, path, len, 0);

  if(!len) {
    printf(COM2, "No route from %s to %s found\n\r", src, dst);
  } else {
    printf(COM2, "Setting route from %s to %s\n\r", src, dst);
  }
}

void handle_create_train_cmd(int num) {
  if(num < 0 || num > MAX_TRAIN) {
    printf(COM2, "Invalid train number\r\n");
  } else if(!trains[num]) {
    printf(COM2, "Creating train %d\r\n", num);
    tr_set_speed(num, 0);
    trains[num] = create_train(num);
  } else
    printf(COM2, "Train already exists!\r\n");
}

void handle_goto_cmd(int num, char *dst, int offset_mm) {
  int td = trains[num];
  if(!td) {
    printf(COM2, "Train %d does not exist\r\n", num);
    return;
  }

  struct position posn;
  train_get_position(td, &posn);

  int len;
  struct track_node *srcn = posn.node;
  struct track_node *dstn = lookup_track_node(dst);
  struct track_node *path[MAX_PATH_LEN];

  if(!srcn) {
    printf(COM2, "srcn returned from get position should not have been null\r\n");
    return;
  }
  if(!dstn) {
    printf(COM2, "%s is not a valid node\r\n", dst);
    return;
  }

  len = path_find(srcn, dstn, path);
  train_set_path(td, path, len, offset_mm);

  if(!len) {
    printf(COM2, "No route from %s to %s found\n\r", posn.node->name, dst);
  } else {
    printf(COM2, "Setting route from %s to %s\n\r", posn.node->name, dst);
  }
}



void handle_help_cmd() {
  putstr(COM2, "Valid Commands: \r\n\r\n");
  printf(COM2, "\ttr <train number> <speed> - Sets the speed of the train\r\n");
  printf(COM2, "\tsw <switch number> <C|S> - Changes the position of the switch to either curved or straight\r\n");
  printf(COM2, "\trv <train number> - Reverses the given train\r\n");
  printf(COM2, "\tq - Exits the program\r\n");
  printf(COM2, "\tset_track <A|B> - Set the track being used (default is A)\r\n");
}
