#ifndef _MOVEMENT_H_
#define _MOVEMENT_H_

#include <train/calibration.h>
#include <routing.h>

#define TIME_FOREVER 0x0fffffff

struct position {
  struct track_node *node;
  int offset;
};
/* I prefer defining enums which are only used inside of a single struct inline */
enum travel_method {
  SHORT_MOVE,
  LONG_MOVE,
};

enum velocity_state {
  ACCELERATING,
  DECELERATING,
  UNIFORM
};

enum direction {
  FORWARD,
  REVERSE
};

struct movement {
  enum velocity_state state;
  int start_velocity;
  int acceleration_update_time;
  int duration;
};

struct movement_state {
  // location and dynamics
  int train_num;
  int speed;
  int is_reversed;
  struct train_calibration calibration;
  struct movement accel;

  int has_path_completed;
  struct track_node *path[MAX_PATH_LEN];
  struct position position;
  struct position position_definite_if_different;
  int path_length;
  int path_index;
  int destination_offset;

  // stopping
  int stopping_time, bsens_stopping_time;
  struct position stop_position, bsens_stop_position;
  int stop_delay_tid, bsens_stop_delay_tid;
  int bsens_timeout_active;

  // speed change
  int speed_change_calc_delay_tid;
  int speed_change_calc_delay_time;
  int delayed_speed;
  int delayed_speed_until;

  // updating
  int update_delay_tid;
  int update_delay_time;
  enum travel_method travel_method;

  // expectation/sensor attribution
  struct track_node *expected_next_sensor;
  int expected_next_sensor_index;
  int dist_to_next_sens;
  enum velocity_state accel_from_last_sensor;
  int last_sensor_time;
  struct track_node *expected_next_sensor_definite_if_different;
  int dist_to_next_sens_definite_if_different;
};

void get_position(struct movement_state *state, struct position *new_position);

void train_reset(struct movement_state *state);
void update_current_quantities(
  struct movement_state *state,
  ///
  enum direction *reverse,
  struct position *position,
  int *velocity,
  int *acceleration,
  struct track_node **path
);

void update_path(
  struct movement_state *state,
  struct track_node **new_path,
  int posn_offset,
  int dest_offset,
  int should_reverse,
  ///
  enum direction *reverse,
  struct position *position,
  int *velocity,
  int *acceleration,
  struct track_node **path
);
void reverse_train(
  struct movement_state *state,
  ///
  enum direction *reverse,
  struct position *position
);
void update_speed(
  struct movement_state *state,
  int speed,
  int until
);
void _update_speed_predictions(struct movement_state *state);
void update_speed_predictions(
  struct movement_state *state,
  ///
  struct position *position,
  int *velocity,
  int *acceleration
);
void handle_accel_finished(
  struct movement_state *state,
  ///
  struct position *position,
  int *velocity,
  int *acceleration
);
void handle_sensors(
  struct movement_state *state,
  struct track_node **sensors,
  ///
  struct position *position,
  int *velocity,
  int *acceleration
);

#endif
