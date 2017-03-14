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

enum travel_method {
  SHORT_MOVE,
  LONG_MOVE,
};

struct train_state {
  // location and dynamics
  int train_num;
  int speed;
  int is_reversed;
  struct train_calibration calibration;

  struct position position;
  struct track_node *path[MAX_PATH_LEN];
  int path_index;

  // stopping
  int stopping_time;
  struct position stop_position;
  int delay_tid;
  enum travel_method travel_method;

  // expectation/sensor attribution
  struct track_node *expected_next_sensor;
  int expected_next_sensor_index;
  int expected_time_at_next_sens;
  int dist_to_next_sens;
  int last_sensor_update;
};

/* API */
void train_set_path(int td, struct track_node **path, int len);
int create_train(int num);

#endif
