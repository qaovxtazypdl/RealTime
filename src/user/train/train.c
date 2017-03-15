#include <common/syscall.h>
#include <track_node.h>
#include <track.h>
#include <train.h>
#include <common/mem.h>
#include <train_io.h>
#include <io.h>
#include <routing.h>
#include <sensors.h>
#include <clock_server.h>

#define DURATION_FOREVER 0x0fffffff
#define SPEEDUP_DELAY 45

unsigned int isqrt(unsigned int x){unsigned int a,b,c;a=x;b=0;c=1<<30;while(c>a)c>>=2;while(c!=0){if(a>=b+c){a-=b+c;b+=c<<1;}b>>=1;c>>=2;}return b;}

void update_position_display(struct train_state *state) {
  printf(COM2,
    "%s%m%scurrent position %d mm past sensor %d. Next sensor: %d\n\r",
    SAVE_CURSOR,
    (int[]){0, 41},
    CLEAR_LINE,
    state->position.offset, state->position.node->num,
    state->expected_next_sensor->num,
    RESTORE_CURSOR
  );
}

void update_sensor_display(struct train_state *state, int delta_t, int delta_d) {
  printf(COM2,
    "%s%m%s train %d ~ delta_t = %d, delta_d = %d mm, next sensor: %d\n\r%s",
    SAVE_CURSOR,
    (int[]){0, 40},
    CLEAR_LINE,
    state->train_num,
    delta_t, delta_d, state->expected_next_sensor->num,
    RESTORE_CURSOR
  );
}

int get_distance_to_next_node_in_path(struct track_node **path, int path_index, int *distance) {
  struct track_node *current = path[path_index];
  int direction = 0;

  if (path_index < 0 || path[path_index] == NULL || path[path_index + 1] == NULL) {
    return -1;
  }

  switch(current->type) {
    case NODE_BRANCH:
      if(path[path_index + 1] && (current->edge[DIR_STRAIGHT].dest == path[path_index + 1])) {
        direction = DIR_STRAIGHT;
      } else {
        direction = DIR_CURVED;
      }
      break;
    case NODE_SENSOR:
    case NODE_MERGE:
    case NODE_ENTER:
      direction = DIR_AHEAD;
      break;
    case NODE_EXIT:
    default:
      return -1;
  }
  *distance = current->edge[direction].dist;
  return path_index + 1;
}

int get_next_sensor_in_path(struct track_node **path, int path_index, int *segment_distance) {
  int distance = 0;
  int retval = 0;
  *segment_distance = 0;
  if (path[path_index] == NULL) {
    return -1;
  }

  struct track_node *current = path[path_index];
  do {
    retval = get_distance_to_next_node_in_path(path, path_index, &distance);
    if (retval < 0) {
      // no more paths
      return -1;
    }
    *segment_distance += distance;
    path_index++;
    current = path[path_index];
  } while (current != NULL && current->type != NODE_SENSOR);

  return path_index;
}

int path_length(struct track_node **path) {
  int i = 0;
  int retval = 0;
  int length = 0;
  int new_distance = 0;
  while (path[i] != NULL && path[i + 1] != NULL) {
    retval = get_distance_to_next_node_in_path(path, i, &new_distance);
    length += new_distance;
    i++;
  }

  return length;
}

int advance_train_by_sensor(struct track_node **path, int path_index, int *offset, int distance) {
  int next_sensor_path_index = path_index;
  int return_sensor = path_index;
  int segment_distance = 0;
  int distance_advanced = 0;
  *offset = 0;

  while (path[next_sensor_path_index] != NULL) {
    next_sensor_path_index = get_next_sensor_in_path(path, next_sensor_path_index, &segment_distance);
    if (next_sensor_path_index < 0 || distance_advanced + segment_distance > distance) {
      *offset = distance - distance_advanced;
      break;
    }

    // passed the distance check - next iteration
    return_sensor = next_sensor_path_index;
    distance_advanced += segment_distance;
  }
  return return_sensor;
}

int min(int a, int b) {
  return a > b ? b : a;
}

void update_acceleration_and_position(struct train_state *state) {
  int current_time = get_time();

  int d = 0;
  int a = 0;
  int v_i = state->accel.start_velocity;

  if (state->accel.state == UNIFORM) {
    a = 0;
  } else if (state->accel.state == ACCELERATING) {
    a = state->calibration.acceleration;
  } else if (state->accel.state == DECELERATING) {
    a = -state->calibration.deceleration;
  }

  int t = min(current_time - state->accel.acceleration_update_time, state->accel.duration);
  d = (v_i * t / 100) + (a * t * t / 2 / 10000);

  if (current_time - state->accel.acceleration_update_time >= state->accel.duration) {
    // finished accelerating... set back to constant velocity
    int v_f = state->calibration.speed_to_velocity[state->speed];
    d += v_f * (current_time - state->accel.acceleration_update_time - state->accel.duration) / 100;

    state->accel.state = UNIFORM;
    state->accel.acceleration_update_time = current_time;
    state->accel.start_velocity = v_f;
    state->accel.duration = DURATION_FOREVER;
  } else {
    int v_f = state->accel.start_velocity + a * min(
      current_time - state->accel.acceleration_update_time, state->accel.duration
    ) / 100;
    state->accel.duration -= (current_time - state->accel.acceleration_update_time);
    state->accel.acceleration_update_time = current_time;
    state->accel.start_velocity = v_f;
  }
  state->position.offset += d;
}

void update_speed(struct train_state *state, int speed, int until) {
  if (speed != state->speed) {
    int current_time = get_time();
    if (state->speed == 0 && speed > 0) {
      current_time += SPEEDUP_DELAY;
    }

    // update position based on old accel profile
    update_acceleration_and_position(state);

    // update acceleration profile to match new speed
    int a = speed > state->speed ? state->calibration.acceleration : -state->calibration.deceleration;
    int old_velocity = state->accel.start_velocity + a * min(
      current_time - state->accel.acceleration_update_time, state->accel.duration
    ) / 100;
    state->accel.state = speed > state->speed ? ACCELERATING : DECELERATING;
    state->accel.start_velocity = old_velocity;
    // it takes some time for the train to register a speed command.
    state->accel.acceleration_update_time = current_time;
    state->accel.duration = 100 *
      (state->calibration.speed_to_velocity[speed] - state->calibration.speed_to_velocity[state->speed]) / a;

    // set speed
    state->speed = speed;
    tr_set_speed(state->train_num, state->speed);
  }

  if (until > 0) {
    state->stop_delay_tid = delay_until_async(until);
  } else {
    state->update_delay_tid = delay_async(state->accel.duration);
  }
}

// returns -1 if stopping position is behind
int update_travel_plans(struct train_state *state) {
  int distance = path_length(state->path) - state->position.offset;
  if (distance < 0) {
    return -1;
  }

  if (distance < 1600) {
    state->travel_method = SHORT_MOVE;

    // model for short moves:
    // distance = 1/2 * k * (t(s)-offset)^2
    /*int a_a = state->calibration.acceleration;
    int a_d = state->calibration.deceleration;
    int k = (a_a + a_a * a_a / a_d) / 2;*/
    int offset = state->calibration.speed_change_time_offset;
    int short_move_ticks = isqrt(10000 * distance / state->calibration.k) + offset;

    state->stopping_time = get_time() + short_move_ticks;
    printf(COM2, "short move parameters determined: ticks=%d, distance=%d", short_move_ticks, distance);
    update_speed(state, 14, state->stopping_time);
    return 0;
  } else {
    state->travel_method = LONG_MOVE;

    update_speed(state, 14, 0);

    int stopping_distance =
      (state->is_reversed ? state->calibration.reverse_offset : state->calibration.forward_offset) +
      (state->calibration.stopping_distance[state->speed]);

    if (distance < stopping_distance) {
      return -1;
    }
    int distance_before_stop = distance - stopping_distance;

    int stop_offset = 0;
    int stop_index = advance_train_by_sensor(state->path, state->path_index, &stop_offset, distance_before_stop);
    state->stop_position.node = state->path[stop_index];
    state->stop_position.offset = stop_offset;

    // if stop at current node, just stop immediately
    if (state->position.node->num == state->stop_position.node->num) {
      return -1;
    }

    printf(COM2, "stopping position determined: %d mm past sensor %d\n\r", state->stop_position.offset, state->stop_position.node->num);
    return 0;
  }
}

int update_sensor_prediction(struct train_state *state) {
  int distance = 0;

  int next_sensor_index = get_next_sensor_in_path(state->path, state->path_index, &distance);
  if (next_sensor_index >= 0) {
    state->expected_next_sensor_index = next_sensor_index;
    state->expected_next_sensor = state->path[next_sensor_index];
  } else {
    state->expected_next_sensor_index = -1;
    state->expected_next_sensor = NULL;
  }

  if (state->expected_next_sensor == NULL) {
    state->dist_to_next_sens = 0;
    return -1;
  } else {
    state->dist_to_next_sens = distance;
    return state->expected_next_sensor->num;
  }
}

void update_path(struct train_state *state, struct track_node **new_path) {
  int i;
  struct track_node **c;
  for (i = 0, c = new_path; *c != NULL; c++, i++) {
    state->path[i] = *c;
  }
  state->path[i] = NULL;

  state->position.node = state->path[0];
  state->position.offset = 0;
  state->path_index = 0;
  update_travel_plans(state);
  path_activate(state->path);
  update_sensor_prediction(state);

  update_sensor_display(state, 0, 0);
  update_position_display(state);
}

int time_to_travel(struct movement *accel, int distance) {
  return 0;
}

void update_position(struct train_state *state, struct track_node **sensors) {
  int i = 0;
  while (sensors != NULL && sensors[i] != NULL) {
    if (sensors[i] == state->expected_next_sensor) {

      // updates offset to current time offset from previous sensor.
      update_acceleration_and_position(state);
      int delta_d = state->position.offset - state->dist_to_next_sens;
      int delta_t = delta_d * 100 / state->accel.start_velocity;

      // corrects the offset to 0 to the current sensor
      state->position.node = state->expected_next_sensor;
      state->position.offset = 0;
      state->path_index = state->expected_next_sensor_index;

      update_sensor_prediction(state);
      update_position_display(state);
      update_sensor_display(state, delta_t, delta_d);

      if (
        state->travel_method == LONG_MOVE &&
        state->position.node->num == state->stop_position.node->num
      ) {
        int distance_until_stop_command = state->stop_position.offset;
        state->stopping_time = get_time() + distance_until_stop_command * 100 / state->calibration.speed_to_velocity[state->speed];
        state->stop_delay_tid = delay_until_async(state->stopping_time);
      }
      break;
    }
    i++;
  }
}

void train_reset(struct train_state *state) {
  int current_time = get_time();

  state->speed = 0;
  state->is_reversed = 0;
  state->update_delay_tid = -1;
  state->stop_delay_tid = -1;

  if (g_is_track_a) {
    // track A : A5
    state->position.node = &g_track[4];
  } else {
    // track B : A1
    state->position.node = &g_track[0];
  }
  state->position.offset = 0;

  state->accel.state = UNIFORM;
  state->accel.start_velocity = 0;
  state->accel.acceleration_update_time = current_time;
  state->accel.duration = DURATION_FOREVER;

  update_position_display(state);
}

void train() {
  int tid;
  struct train_state state;

  receive(&tid, &state.train_num, sizeof(state.train_num));
  reply(tid, NULL, 0);

  train_reset(&state);
  init_calibration(&state.calibration, state.train_num);

  union {
    struct track_node *sensors[NUM_SENSORS];
    struct train_command command;
  } msg;

  int sens_tid = sensor_subscribe();

  while(1) {
    receive(&tid, &msg, sizeof(msg));
    reply(tid, NULL, 0);

    if(tid == sens_tid) {
      update_position(&state, msg.sensors);
    } else if(tid == state.stop_delay_tid) {
      if (state.stopping_time <= get_time()) {
        update_speed(&state, 0, 0);
      }
    } else if (tid == state.update_delay_tid) {
      if (state.accel.acceleration_update_time + state.accel.duration) {
        update_acceleration_and_position(&state);
      }
    } else {
      switch(msg.command.type) {
        case TRAIN_COMMAND_SET_PATH:
          update_path(&state, msg.command.path);
          break;
        default:
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

void train_set_path(int tid, struct track_node **path, int len) {
  struct train_command msg;
  int path_sz = (len + 1) * sizeof(struct track_node*);

  msg.type = TRAIN_COMMAND_SET_PATH;
  memcpy(msg.path, path, path_sz);
  send(tid, &msg, path_sz + sizeof(msg.type), NULL, 0);
}

void train_get_location(struct track_node *node, int offset);
