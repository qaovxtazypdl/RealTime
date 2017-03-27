#include <common/syscall.h>
#include <track_node.h>
#include <track.h>
#include <train.h>
#include <common/mem.h>
#include <train_io.h>
#include <console.h>
#include <io.h>
#include <routing.h>
#include <sensors.h>
#include <clock_server.h>
#include <track_node.h>
#include <movement.h>
#include <switch.h>

static int num_trains_initialized = 0;

void train() {
 /* These quantities should be authoritative. */
  int tid;
  int num;
  int line = num_trains_initialized++;

  // physically updated quantities
  enum direction reverse;
  int velocity;
  int acceleration;
  struct position position;
  struct track_node **path;

  // internal movement state
  struct movement_state state;

  receive(&tid, &num, sizeof(state.train_num));
  state.train_num = num;
  reply(tid, NULL, 0);

  train_reset(&state);
  init_calibration(&state.calibration, state.train_num);
  update_current_quantities(&state, &reverse, &position, &velocity, &acceleration, path);

  union {
    struct track_node *sensors[NUM_SENSORS];
    struct train_command command;
  } msg;

  int sens_tid = sensor_subscribe();
  int current_time = get_time();
  while(1) {
    update_train_info(num, line, reverse, &position, velocity, acceleration);
    receive(&tid, &msg, sizeof(msg));
    current_time = get_time();
    update_current_quantities(&state, &reverse, &position, &velocity, &acceleration, path);

    if(tid == sens_tid) {
      reply(tid, NULL, 0);
      handle_sensors(&state, msg.sensors, &position, &velocity, &acceleration);
    } else if(
      tid == state.stop_delay_tid ||
      tid == state.bsens_stop_delay_tid ||
      tid == state.update_delay_tid ||
      tid == state.speed_change_calc_delay_tid) {
      reply(tid, NULL, 0);

      if (
        state.stopping_time <= current_time ||
        (state.bsens_timeout_active && state.bsens_stopping_time <= current_time)
      ) {
        update_speed(&state, 0, 0);
        state.stopping_time = TIME_FOREVER;
        state.bsens_stopping_time = TIME_FOREVER;
        state.bsens_timeout_active = 0;
      }

      if (state.update_delay_time <= current_time) {
        handle_accel_finished(&state, &position, &velocity, &acceleration);
      }

      if (state.speed_change_calc_delay_time <= current_time) {
        update_speed_predictions(&state, &position, &velocity, &acceleration);
      }
    } else {
      int should_reverse = 0;
      struct position new_position;
      switch(msg.command.type) {
        case TRAIN_COMMAND_SET_PATH:
          reply(tid, NULL, 0);
          should_reverse = 0;

          update_path(
            &state,
            msg.command.path,
            position.offset,
            msg.command.offset,
            should_reverse,
            &reverse,
            &position,
            &velocity,
            &acceleration,
            path
          );

          break;
        case TRAIN_COMMAND_GET_POSITION:
          get_position(&state, &new_position);
          reply(tid, &new_position, sizeof(struct position));
          break;
        case TRAIN_COMMAND_REVERSE_TEST:
          reply(tid, NULL, 0);
          reverse_train(&state, &reverse, &position);
          break;
        default:
          reply(tid, NULL, 0);
          break;
      }
    }
  }
}

/* Functions for communicating with a train task. */
int create_train(int num) {
  int tid = create(0, train);
  send(tid, &num, sizeof(num), NULL, 0);
  return tid;
}

void train_set_path(int tid, struct track_node **path, int len, int offset) {
  struct train_command msg;
  int path_sz = (len + 1) * sizeof(struct track_node*);

  msg.type = TRAIN_COMMAND_SET_PATH;
  msg.offset = offset;
  memcpy(msg.path, path, path_sz);
  send(tid, &msg, path_sz + sizeof(msg.type) + sizeof(msg.offset), NULL, 0);
}

/* Just call this train_position, the word 'get' is a gratuitious verb that smells of a particular
   object oriented 'enterprise' language that shall remain nameless ;). */

void train_get_position(int tid, struct position *posn) {
  struct train_command msg;
  msg.type = TRAIN_COMMAND_GET_POSITION;

  send(tid, &msg, sizeof(msg.type), posn, sizeof(struct position));
}

void train_test_reverse(int tid) {
  struct train_command msg;
  msg.type = TRAIN_COMMAND_REVERSE_TEST;

  send(tid, &msg, sizeof(msg.type), NULL, 0);
}

