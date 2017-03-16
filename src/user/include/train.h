#ifndef _TRAIN_H_
#define _TRAIN_H_

#include <track_node.h>
#include <train/calibration.h>
#include <routing.h>

#define TRAIN_COM COM1
#define MAX_TRAIN 200

struct position {
  struct track_node *node;
  int offset;
};

struct train_command {
  enum train_command_type {
    TRAIN_COMMAND_SET_PATH,
    TRAIN_COMMAND_GET_POSITION,
    TRAIN_COMMAND_GOTO
  } type;

  int offset;
  struct track_node *path[MAX_PATH_LEN];
};

/* API */
void train_set_path(int tid, struct track_node **path, int len, int offset);
void train_get_position(int tid, struct position *posn);
int create_train(int num);

#endif
