#ifndef _TRAIN_H_
#define _TRAIN_H_

#include <track_node.h>
#include <train/calibration.h>
#include <routing.h>
#include <movement.h>

#define TRAIN_COM COM1
#define MAX_TRAIN 200

struct train_command {
  enum train_command_type {
    TRAIN_COMMAND_SET_PATH,
    TRAIN_COMMAND_GET_POSITION,
    TRAIN_COMMAND_REVERSE_TEST,
    TRAIN_COMMAND_NOTIFY_RESERVATION,
    TRAIN_COMMAND_GOTO
  } type;

  int offset;
  struct track_node *path[MAX_PATH_LEN];
  struct track_node *reservation;
};

/* API */
void train_set_path(int tid, struct track_node **path, int len, int offset);
void train_get_position(int tid, struct position *posn);
int create_train(int num);
void train_new_reservation_available(int tid, struct track_node *track);

#endif
