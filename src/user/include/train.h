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
    TRAIN_COMMAND_SET_PATH
  } type;

  struct track_node *path[MAX_PATH_LEN];
};

struct train_state {
  int train_num;
  int speed;
  int is_reversed;

  int stopping_time;
  struct position stop_position;
  int delay_tid;

  struct position position;
  struct track_node *path[MAX_PATH_LEN];
  int path_index;

  struct track_node *expected_next_sensor;
  int expected_next_sensor_index;
  int expected_time_at_next_sens;
  int dist_to_next_sens;
  int last_sensor_update;

  struct train_calibration calibration;

};

/* API */
void train_set_path(int td, struct track_node **path, int len);
int create_train(int num);

#endif
