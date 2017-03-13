#include <common/syscall.h>
#include <track_node.h>
#include <track.h>
#include <train.h>
#include <train_io.h>
#include <io.h>
#include <routing.h>
#include <sensors.h>
#include <calibration.h>
#include <clock_server.h>

static void update_path(struct track_node **path, struct track_node **new_path) {
  int i;
  struct track_node **c;

  for (i = 0, c = new_path; *c != NULL; c++, i++)
    path[i] = *c;

  path[i] = NULL;
  path_activate(path);
}

static void update_speed(int *speed) {
  //TODO implement
  *speed = 14;
  tr_set_speed(state.train_num, *speed);
}

static void update_position(struct track_node **sensors, struct position *position, int last_sensor_update) {
  //TODO implement.
}

static void update_stopping_time(struct track_node **sensors,
  int *stopping_time,
  int speed,
  struct position position,
  struct track_node **path
) {
}

static void train_reset(struct train_state *state) {
  state->speed = 0;
  state->is_reversed = 0;

  if (g_is_track_a) {
    // track A : A5
    state->position.node = &g_track[4];
    state->expected_next_sens_node = &g_track[38];
  } else {
    // track B : A1
    state->position.node = &g_track[0];
    state->expected_next_sens_node = &g_track[44];
  }
  state->position.offset = 0;
  last_sensor_update = get_time();

  state->destination.node = NULL;
  state->destination.offset = 0;
}

static void train() {
  int tid;
  struct train_state state;

  receive(&td, &state->train_num, sizeof(state->train_num));
  reply(td, NULL, 0);

  train_reset(&state);
  init_calibration(&state.calibration, train_num);

  union {
    struct track_node *sensors[NUM_SENSORS];
    struct train_command command;
  } msg;

  int sens_td = sensor_subscribe();
  int delay_td = -1;

  last_update = get_time();
  while(1) {
    receive(&td, &msg, sizeof(msg));
    reply(td, NULL, 0);

    if(tid == sens_tid) {
      update_speed(&state.speed);
      update_position(msg.sensors, &state.position, state.last_sensor_update);
      update_stopping_time(msg.sensors, &state.stopping_time, state.speed, state.position, path);
      state.last_sensor_update = get_time();
      delay_tid = delay_until_async(state.stopping_time);
    } else if(tid == delay_tid) {
      //CHECK stopping time against current time and stop if equal.
    } else {
      switch(msg.command.type) {
        case TRAIN_COMMAND_SET_PATH:
          update_path(state.path, msg.command.path);
          break;
        default:
          break;
      }
    }

  }
}

/* Functions for communicating with a train task. */
int create_train(int num) {
  int td = create(0, train);
  send(td, &num, sizeof(num), NULL, 0);
  return td;
}

void train_set_path(int td, struct track_node **path, int len) {
  struct train_command msg;
  struct track_node **n;
  int path_sz = (len + 1) * sizeof(struct track_node*);

  msg.type = TRAIN_COMMAND_SET_PATH;
  memcpy(msg.path, path, path_sz);
  send(td, &msg, path_sz + sizeof(msg.type), NULL, 0);
}
