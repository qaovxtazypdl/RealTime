#include <common/syscall.h>
#include <track_node.h>
#include <track.h>
#include <train.h>
#include <common/mem.h>
#include <train_io.h>
#include <console.h>
#include <io.h>
#include <sensors.h>
#include <clock_server.h>
#include <track_node.h>
#include <switch.h>
#include <movement.h>

static int current_debug_line = 0;

#define TIME_FOREVER 0x0fffffff
#define SENSOR_ATTRIBUTION_DISTANCE_TOLERANCE 400

/* I am going to pretend I didn't see this. */
inline unsigned int isqrt(unsigned int x){
  unsigned int a,b,c;
  a=x;
  b=0;
  c=1<<30;
  while(c>a)
    c>>=2;
  while(c!=0){
    if(a>=b+c){
      a-=b+c;
      b+=c<<1;
    }
    b>>=1;
    c>>=2;
  }
  return b;
}

/* Make these macros and move them into math.h */

inline int min(int a, int b) {
  return a > b ? b : a;
}
inline int max(int a, int b) {
  return a < b ? b : a;
}
inline int abs(int a) {
  return a < 0 ? -a : a;
}

void get_position(struct movement_state *state, struct position *new_position);
inline void update_current_position(struct movement_state *state, struct position *position) {
  get_position(state, position);
}
inline void update_current_velocity(struct movement_state *state, int *velocity) {
  int current_time = get_time();
  int accel;

  if (state->accel.state == UNIFORM) {
    accel = 0;
  } else if (state->accel.state == ACCELERATING) {
    accel = state->calibration.acceleration;
  } else if (state->accel.state == DECELERATING) {
    accel = -state->calibration.deceleration;
  }

  // assumption - time not yet past the current accel update time
  *velocity = state->accel.start_velocity + accel * min(
    current_time - state->accel.acceleration_update_time, state->accel.duration
  ) / 100;
}
inline void update_current_acceleration(struct movement_state *state, int *acceleration) {
  if (state->accel.state == UNIFORM) {
    *acceleration = 0;
  } else if (state->accel.state == ACCELERATING) {
    *acceleration = state->calibration.acceleration;
  } else if (state->accel.state == DECELERATING) {
    *acceleration = -state->calibration.deceleration;
  }
}
inline void update_current_path(struct movement_state *state, struct track_node **path) {
  path = state->path;
}
inline void update_current_reverse(struct movement_state *state, enum direction *reverse) {
  *reverse = state->is_reversed ? REVERSE : FORWARD;
}

void update_current_quantities(
  struct movement_state *state,
  enum direction *reverse,
  struct position *position,
  int *velocity,
  int *acceleration,
  struct track_node **path
) {
  update_current_position(state, position);
  update_current_velocity(state, velocity);
  update_current_acceleration(state, acceleration);
  update_current_path(state, path);
  update_current_reverse(state, reverse);
}

/* General comment about functions: */

/* Keep function names in shared code concise, avoid prefixing things
   with 'get'.  'distance_to' might be a good option in this case. If
   necessary add a comment at the top of the function describing what
   it does.  If your function does something other than return 0 on
   success, document this explicitly. Make all of the pointers to
   things which are not modified, const (I need to start doing this
   too) so it is clear which arguments (if any) get modified. For this
   function I would write something like 'Returns the path_index + 1
   and stores distance in the pointer it gets passed'. When possible a
   function should return the thing of interest rather than modify
   stack space in the caller unless it is essential to return multiple
   values. In this case I would just return distance or -1 in the case
   of failure' */

int get_distance_to_next_node_in_path(struct track_node **path, int path_index, int *distance) {
/* Don't forget to check if path is NULL. */
  struct track_node *current = path[path_index];
  int direction = 0;
  *distance = 0;

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
/* case NODE_EXIT is gratuitous here, it is already covered by default */
    case NODE_EXIT:
    default:
      return -1;
  }
  *distance = current->edge[direction].dist;
  return path_index + 1;
}

/* See above. */
int get_prev_sensor_in_path(struct track_node **path, int path_index, int *segment_distance) {
  int distance = 0;
  int retval = 0;
  *segment_distance = 0;

  if (path[path_index] == NULL || path[path_index] <= 0) {
    return -1;
  }

/* Put declarations at the top. */
  struct track_node *current = path[path_index];
  do {
    path_index--;
    current = path[path_index];
    retval = get_distance_to_next_node_in_path(path, path_index, &distance);
    if (retval < 0) {
      // no more paths
      return -1;
    }
    *segment_distance += distance;
  } while (path_index > 0 && current->type != NODE_SENSOR);

  return path_index;
}

/* See above. */
int get_next_sensor_in_path(struct track_node **path, int path_index, int *segment_distance) {
  int distance = 0;
  int retval = 0;
  *segment_distance = 0;

/* This check isn't useful, if path_index is valid it should not be
pointing to NULL, if it is invalid (i.e exceeds the length of the
buffer) then it could point to anything.  Instead check if path itself
is NULL or *path is NULL (which is equivalent to path[0] == NULL and
means the path is empty).  */

  if (path[path_index] == NULL) {
    return -1;
  }

/* Put declarations at the top. */
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

/* I would prefer to call this path_distance, length usually refers to the size
of a buffer in C. */
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
/* It's no*/
int advance_train_by_sensor(struct track_node **path, int path_index, int *offset, int distance) {
  int next_sensor_path_index = path_index;
  int return_sensor = path_index;
  int segment_distance = 0;
  int distance_advanced = 0;
  *offset = 0;

  while (path[next_sensor_path_index] != NULL) {

/* In general, try and delimit lines longer than 80 characters with a
newline. If the line is slightly over (3-8 chars) and it improves
readability that's fine. Shorter function names help facilitate this. */

    next_sensor_path_index = get_next_sensor_in_path(path, next_sensor_path_index, &segment_distance);

    /* 'distance_advanced + segment_distance > distance' should be
'(distance_advanced + segment_distance) > distance'. I'm assuming this
is correct but my tiny brain doesn't have room for precedence
rules. This applies everywhere, but especially where you use
the ternary operator. */

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

int get_offset_from_current_position(struct movement_state *state) {
  int current_time = get_time();
  if (state->has_path_completed || (state->accel.state == UNIFORM && state->speed == 0)) {
    return 0;
  }
  if (current_time <= state->accel.acceleration_update_time) {
    return 0;
  }

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
    int v_f = state->speed == 0 ? 0 : state->calibration.speed_to_velocity[state->speed];
    d += v_f * (current_time - state->accel.acceleration_update_time - state->accel.duration) / 100;
  }
  return d;
}

void get_position(struct movement_state *state, struct position *new_position) {
  int d = get_offset_from_current_position(state);
  new_position->node = state->position.node;
  new_position->offset = d + state->position.offset;
}
void update_acceleration_and_position(struct movement_state *state) {
  int current_time = get_time();
  if (current_time < state->accel.acceleration_update_time) {
    return;
  }

  int d = get_offset_from_current_position(state);

  int a = 0;
  if (state->accel.state == UNIFORM) {
    a = 0;
  } else if (state->accel.state == ACCELERATING) {
    a = state->calibration.acceleration;
  } else if (state->accel.state == DECELERATING) {
    a = -state->calibration.deceleration;
  }

  if (current_time - state->accel.acceleration_update_time >= state->accel.duration) {
    // finished accelerating... set back to constant velocity
    int v_f = state->speed == 0 ? 0 : state->calibration.speed_to_velocity[state->speed];

    state->accel.state = UNIFORM;
    state->accel.acceleration_update_time = current_time;
    state->accel.start_velocity = v_f;
    state->accel.duration = TIME_FOREVER;
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
/* Consumes */
void _update_speed(struct movement_state *state, int speed, int until) {
  if (speed != state->speed) {
    int current_time = get_time();
    if (state->speed == 0 && speed > 0 && state->accel.state == UNIFORM) {
      current_time += state->calibration.startup_time;
    }

    // update position based on old accel profile
    update_acceleration_and_position(state);

    // update acceleration profile to match new speed
    int a = speed > state->speed ? state->calibration.acceleration : -state->calibration.deceleration;
    int old_velocity = state->accel.start_velocity + a * min(
      current_time - state->accel.acceleration_update_time, state->accel.duration
    ) / 100;
    state->accel.state = speed > state->speed ? ACCELERATING : DECELERATING;
    state->accel_from_last_sensor = speed > state->speed ? ACCELERATING : DECELERATING;
    state->accel.start_velocity = old_velocity;
    // it takes some time for the train to register a speed command.
    state->accel.acceleration_update_time = current_time;
    state->accel.duration = 100 *
      (state->calibration.speed_to_velocity[speed] - old_velocity) / a;

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
void update_speed(
  struct movement_state *state,
  int speed,
  int until,
  struct position *position,
  int *velocity,
  int *acceleration
) {
  _update_speed(state, speed, until);

  update_current_position(state, position);
  update_current_velocity(state, velocity);
  update_current_acceleration(state, acceleration);
}

// returns -1 if stopping position is behind
int update_travel_plans(struct movement_state *state) {
  int distance = path_length(state->path) - state->position.offset + state->destination_offset;
  if (distance < 0) {
    return -1;
  }

  if (distance < 1600) {
    state->travel_method = SHORT_MOVE;

    // model for short moves:
    // d = k*(t-offset)^2/10000 + b
    // sqrt[(d - b)/k * 10000] + offset

    // distance = 1/2 * k * (t(s)-offset)^2
    /*int a_a = state->calibration.acceleration;
    int a_d = state->calibration.deceleration;
    int k = (a_a + a_a * a_a / a_d) / 2;*/
    int offset = state->calibration.speed_change_time_offset;
    int short_move_ticks;
    if (distance > state->calibration.b) {
      short_move_ticks = isqrt(10000 * (distance - state->calibration.b) / state->calibration.k) + offset;
    } else {
      short_move_ticks = offset;
    }

    state->stopping_time = get_time() + short_move_ticks;
    printf(COM2, "short move parameters determined: ticks=%d, distance=%d", short_move_ticks, distance);

    _update_speed(state, 14, state->stopping_time);
    return 0;
  } else {
    state->travel_method = LONG_MOVE;
    // wtf (this is needed)
    distance += state->position.offset;
    _update_speed(state, 12, 0);

    int sensor_attribute_offset = state->is_reversed ? state->calibration.reverse_offset : state->calibration.forward_offset;
    int stopping_distance = state->calibration.stopping_distance[state->speed] + sensor_attribute_offset;
    if (distance < stopping_distance) {
      return -1;
    }
    int distance_before_stop = distance - stopping_distance;

    int stop_offset = 0;
    int stop_index = advance_train_by_sensor(state->path, state->path_index, &stop_offset, distance_before_stop);
    stop_offset += sensor_attribute_offset;
    state->stop_position.node = state->path[stop_index];
    state->stop_position.offset = stop_offset;

    int bsens_stop_offset = 0;
    int prev_index = get_prev_sensor_in_path(state->path, stop_index, &bsens_stop_offset);
    bsens_stop_offset += stop_offset;
    state->bsens_stop_position.node = state->path[prev_index];
    state->bsens_stop_position.offset = bsens_stop_offset;

    // if stop at current node, just stop immediately
    // TODO - jzhao handle this better
    if (state->position.node->num == state->stop_position.node->num) {
      return -1;
    }

    /*printf(COM2, "stopping position determined: %d mm past sensor %s (backup %d mm past sensor %s) \n\r",
      state->stop_position.offset, state->stop_position.node->name,
      state->bsens_stop_position.offset, state->bsens_stop_position.node->name
    );*/
    return 0;
  }
}

int update_sensor_prediction(struct movement_state *state) {
  int distance = 0;

  int next_sensor_index = get_next_sensor_in_path(state->path, state->path_index, &distance);
  if (next_sensor_index >= 0) {
    state->expected_next_sensor_index = next_sensor_index;
    state->expected_next_sensor = state->path[next_sensor_index];
  } else {
    state->expected_next_sensor_index = -1;
    state->expected_next_sensor = NULL;
  }

  state->last_sensor_time = get_time();
  if (state->expected_next_sensor == NULL) {
    state->dist_to_next_sens = 0;
    return -1;
  } else {
    state->dist_to_next_sens = distance;
    return state->expected_next_sensor->num;
  }
}

void _reverse_train(struct movement_state *state);
void _update_path(struct movement_state *state, struct track_node **new_path, int posn_offset, int dest_offset, int should_reverse) {
  int i;
  struct track_node **c;
  for (i = 0, c = new_path; *c != NULL; c++, i++) {
    state->path[i] = *c;
  }
  state->path[i] = NULL;

  // ignore empty paths
  if (i == 0) {
    return;
  }

  current_debug_line = 0;

  // if source node has changed from current position,
  state->position.node = state->path[0];
  state->position.offset = posn_offset;
  state->path_index = 0;
  state->path_length = i;
  state->destination_offset = dest_offset;
  state->has_path_completed = 0;

  if (should_reverse) {
    _reverse_train(state);
    state->expected_next_sensor_definite_if_different = NULL;
    state->position_definite_if_different.node = NULL;
    state->position_definite_if_different.offset = 0;
    state->dist_to_next_sens_definite_if_different = 0;
  }

  update_travel_plans(state);
  path_activate(state->path);
  update_sensor_prediction(state);

  update_sensor_display(state, 0, 0, current_debug_line++);
}
void update_path(
  struct movement_state *state,
  struct track_node **new_path,
  int posn_offset,
  int dest_offset,
  int should_reverse,
  enum direction *reverse,
  struct position *position,
  int *velocity,
  int *acceleration,
  struct track_node **path
) {
  _update_path(state, new_path, posn_offset, dest_offset, should_reverse);

  update_current_reverse(state, reverse);
  update_current_position(state, position);
  update_current_velocity(state, velocity);
  update_current_acceleration(state, acceleration);
  update_current_path(state, path);
}

struct possible_broken_node {
  struct track_node *sensor;
  struct track_node *broken;
  int index;
  int switch_direction;
  int distance;
};
int _get_broken_switch_next_sensors(
  struct track_node *node,
  struct track_node *expected_sens,
  struct possible_broken_node broken[6],
  int *length,
  int d,
  struct track_node *last_branch,
  int last_branch_direction
) {
  int start_index, end_index;
  int brs_result, brc_result;
  int i;
  switch(node->type) {
    case NODE_BRANCH:
      start_index = *length;
      brs_result = _get_broken_switch_next_sensors(node->edge[DIR_STRAIGHT].dest, expected_sens, broken, length, d + node->edge[DIR_STRAIGHT].dist, node, DIR_STRAIGHT);
      brc_result = _get_broken_switch_next_sensors(node->edge[DIR_CURVED].dest, expected_sens, broken, length, d + node->edge[DIR_CURVED].dist, node, DIR_CURVED);
      end_index = *length;

      // correct broken branch
      if (brs_result || brc_result) {
        for(i = start_index; i < end_index; i++) {
          broken[*length].broken = node;
        }
      }
      return 0;
      break;
    case NODE_MERGE:
    case NODE_ENTER:
      return _get_broken_switch_next_sensors(node->edge[DIR_AHEAD].dest, expected_sens, broken, length, d + node->edge[DIR_AHEAD].dist, last_branch, last_branch_direction);
      break;
    case NODE_SENSOR:
      if (last_branch != NULL) {
        broken[*length].sensor = node;
        broken[*length].broken = last_branch;
        broken[*length].distance = d;
        broken[*length].switch_direction = last_branch_direction;
        *length = *length + 1;
      }
      return node == expected_sens ? 1 : 0;
      break;
    case NODE_EXIT:
    case NODE_NONE:
      break;
  }
  return 0;
}

// skipped a sensor
// switch broken (with uncertainty in future switches)
// normal expected next sensor
// returns number of sensors gotten
int get_broken_switch_next_sensors(struct track_node *node, struct track_node *expected_sens, struct possible_broken_node broken[6]) {
  int length = 0;
  switch(node->type) {
    case NODE_BRANCH:
      _get_broken_switch_next_sensors(node->edge[DIR_STRAIGHT].dest, expected_sens, broken, &length, node->edge[DIR_STRAIGHT].dist, node, DIR_STRAIGHT);
      _get_broken_switch_next_sensors(node->edge[DIR_CURVED].dest, expected_sens, broken, &length, node->edge[DIR_CURVED].dist, node, DIR_CURVED);
      break;
    case NODE_MERGE:
    case NODE_ENTER:
    case NODE_SENSOR:
      _get_broken_switch_next_sensors(node->edge[DIR_AHEAD].dest, expected_sens, broken, &length, node->edge[DIR_AHEAD].dist, NULL, DIR_STRAIGHT);
      break;
    case NODE_EXIT:
    case NODE_NONE:
      break;
  }
  broken[length].sensor = NULL;
  return length;
}

void get_broken_sensor_next_sensor(struct track_node **path, int path_index, struct possible_broken_node *broken) {
  int next_dist = 0;
  int next_sensor_seen = 0;
  broken->sensor = NULL;
  broken->distance = 0;

  get_distance_to_next_node_in_path(path, path_index, &next_dist);
  broken->distance += next_dist;
  path_index++;

  while(path[path_index] != NULL) {
    if (path[path_index]->type == NODE_SENSOR) {
      if (next_sensor_seen) {
        broken->sensor = path[path_index];
        broken->index = path_index;
        break;
      } else {
        next_sensor_seen = 1;
        broken->broken = path[path_index];
      }
    }

    get_distance_to_next_node_in_path(path, path_index, &next_dist);
    broken->distance += next_dist;
    path_index++;
  }
}

void _handle_sensors(struct movement_state *state, struct track_node **sensors) {
  int i = 0;
  if (sensors[0] == NULL) {
    return;
  }

  int current_time = get_time();

  struct possible_broken_node broken_switch[6];
  struct possible_broken_node broken_sensor;

  get_broken_sensor_next_sensor(state->path, state->path_index, &broken_sensor);
  int bsw_sens_count = get_broken_switch_next_sensors(
    state->position_definite_if_different.node != NULL ? state->position_definite_if_different.node : state->position.node,
    state->expected_next_sensor_definite_if_different != NULL ? state->expected_next_sensor_definite_if_different : state->expected_next_sensor,
    broken_switch
  );

/*
  printf(COM2, "bsens: %s, bsensdist: %d\n\r",  broken_sensor.sensor == NULL ? "x" : broken_sensor.sensor->name, broken_sensor.distance);
  int dbg = 0;
  for (dbg = 0; dbg < bsw_sens_count; dbg++) {
    printf(COM2, "bsw_i=%d, bsw: %s, bsw_d: %d\n\r", dbg, broken_switch[dbg].sensor->name, broken_switch[dbg].distance);
  }
*/
  int d_offset_from_current_node = get_offset_from_current_position(state) + state->position.offset;
  int d_offset_from_prev_sensor_if_different = get_offset_from_current_position(state) +
    (state->position_definite_if_different.node != NULL ? state->position_definite_if_different.offset : state->position.offset);
  int delta_d = 0;
  int attributed_sensor_index = 0;
  struct track_node *attributed_sensor = NULL;
  int sensor_attribute_offset = state->is_reversed ? state->calibration.reverse_offset : state->calibration.forward_offset;

  while (sensors != NULL && sensors[i] != NULL) {
    /*if (sensors[i]->num == 71) {
      printf(COM2, "ignoring sensor E8.");
    } else */
    if (sensors[i] == state->expected_next_sensor) {
      delta_d = d_offset_from_current_node - (state->dist_to_next_sens + sensor_attribute_offset);
      if (abs(delta_d) < SENSOR_ATTRIBUTION_DISTANCE_TOLERANCE) {
        attributed_sensor = sensors[i];
        attributed_sensor_index = state->expected_next_sensor_index;
      }
    } else if (sensors[i] == broken_sensor.sensor) {
      delta_d = d_offset_from_current_node - (broken_sensor.distance + sensor_attribute_offset);
      if (abs(delta_d) < SENSOR_ATTRIBUTION_DISTANCE_TOLERANCE) {
        // expected sensor was broken!
        attributed_sensor = sensors[i];
        attributed_sensor_index = broken_sensor.index;

        sensor_register_broken(broken_sensor.broken);
        print_broken(broken_sensor.broken->name, current_debug_line++);
      }
    } else {
      // check for broken switch and determine exactly which switch was broken
      int broken_it;
      for (broken_it = 0; broken_it < bsw_sens_count; broken_it++) {
        delta_d = d_offset_from_prev_sensor_if_different - (broken_switch[broken_it].distance + sensor_attribute_offset);
        if (
          broken_switch[broken_it].sensor != state->position.node &&
          sensors[i] == broken_switch[broken_it].sensor &&
          abs(delta_d) < SENSOR_ATTRIBUTION_DISTANCE_TOLERANCE
        ) {
          attributed_sensor = sensors[i];
          attributed_sensor_index = 0;

// TODO: jzhao - HACK - replace this with nonblocking
          int len;
          struct track_node *srcn = attributed_sensor;
          struct track_node *dstn = state->path[state->path_length - 1];
          struct track_node *path[MAX_PATH_LEN];
          len = path_find(srcn, dstn, path);
          _update_path(state, path, sensor_attribute_offset, state->destination_offset, 0);

          struct broken_switch broken_updater;
          broken_updater.node = broken_switch[broken_it].broken;
          broken_updater.position = broken_switch[broken_it].switch_direction;

          switch_register_broken(&broken_updater);
          print_broken(broken_switch[broken_it].broken->name, current_debug_line++);

          break;
        }
      }
    }

    if (attributed_sensor != NULL) {
      // updates offset to current time offset from previous sensor.
      int original_offset = state->position.offset;
      update_acceleration_and_position(state);
      int delta_t = delta_d * 100 / state->accel.start_velocity;

      // adaptive velocity calibration if we're going at uniform velocity
      if (state->accel_from_last_sensor == UNIFORM) {
        int actual_velocity = (sensor_attribute_offset + state->dist_to_next_sens - original_offset) * 100 / (current_time - state->last_sensor_time);
        int expected_velocity = state->calibration.speed_to_velocity[state->speed];
        // if within 20%, adjust with adjustment factor 0.2
        if (
          (expected_velocity < actual_velocity && ((actual_velocity - expected_velocity) * 5 < expected_velocity)) ||
          (expected_velocity >= actual_velocity && ((expected_velocity - actual_velocity) * 5 < expected_velocity))
        ) {
          state->calibration.speed_to_velocity[state->speed] = (0.8 * expected_velocity) + (0.2 * actual_velocity);
          state->accel.start_velocity = state->calibration.speed_to_velocity[state->speed];
        }
      }

      // corrects the offset to 0 to the current sensor
      state->position.node = attributed_sensor;
      state->position.offset = sensor_attribute_offset;
      state->path_index = attributed_sensor_index;

      update_sensor_prediction(state);
      update_sensor_display(state, delta_t, delta_d, current_debug_line++);

      state->accel_from_last_sensor = state->accel.state;

      if (
        state->travel_method == LONG_MOVE &&
        state->position.node == state->stop_position.node
      ) {
        int distance_until_stop_command = state->stop_position.offset - state->position.offset;
        state->stopping_time = current_time + distance_until_stop_command * 100 / state->calibration.speed_to_velocity[state->speed];
        state->stop_delay_tid = delay_until_async(state->stopping_time);
        state->bsens_timeout_active = 0;
      }

      if (
        state->travel_method == LONG_MOVE &&
        state->position.node == state->bsens_stop_position.node
      ) {
        int distance_until_stop_command = state->bsens_stop_position.offset - state->position.offset;
        state->bsens_stopping_time = current_time + distance_until_stop_command * 100 / state->calibration.speed_to_velocity[state->speed];
        state->bsens_stop_delay_tid = delay_until_async(state->bsens_stopping_time);
        state->bsens_timeout_active = 1;
      }

      // get rid of definite sensors
      state->position_definite_if_different.node = NULL;
      state->position_definite_if_different.offset = 0;
      state->expected_next_sensor_definite_if_different = NULL;
      state->dist_to_next_sens_definite_if_different = 0;
      break;
    }
    i++;
  }
}
void handle_sensors(
  struct movement_state *state,
  struct track_node **sensors,
  struct position *position,
  int *velocity,
  int *acceleration
) {
  _handle_sensors(state, sensors);

  update_current_position(state, position);
  update_current_velocity(state, velocity);
  update_current_acceleration(state, acceleration);
}

void train_reset(struct movement_state *state) {
  int current_time = get_time();

  state->speed = 0;
  state->is_reversed = 0;
  state->update_delay_tid = -1;
  state->stop_delay_tid = -1;
  state->bsens_stop_delay_tid = -1;
  state->has_path_completed = 1;

  if (g_is_track_a) {
    // track A : A5

    /* Avoid accessing the track graph by index.  If you must set the
train to a specific location use lookup_track_node and pass in the
name to obtain the track_node pointer.  This is expensive, but you
should not be doing it in critical code anyway. */

    state->position.node = &g_track[4];
  } else {
    // track B : A1
    state->position.node = &g_track[0];
  }
  state->position.offset = 0;
  state->expected_next_sensor_definite_if_different = NULL;
  state->position_definite_if_different.node = NULL;
  state->position_definite_if_different.offset = 0;
  state->dist_to_next_sens_definite_if_different = 0;

  state->accel.state = UNIFORM;
  state->accel.start_velocity = 0;
  state->accel.acceleration_update_time = current_time;
  state->accel.duration = TIME_FOREVER;
  state->accel_from_last_sensor = UNIFORM;
}

void finish_path(struct movement_state *state) {
  int delta_d = state->position.offset - (state->dist_to_next_sens + state->destination_offset);
  int delta_t = delta_d * 100 / state->accel.start_velocity;

  if (state->path[state->path_length - 1] != state->position.node) {
    state->expected_next_sensor_definite_if_different = state->expected_next_sensor;
    state->position_definite_if_different.node = state->position.node;
    state->position_definite_if_different.offset = state->position.offset;
    state->dist_to_next_sens_definite_if_different = state->dist_to_next_sens;

    state->position.node = state->path[state->path_length - 1];
    state->position.offset = delta_d + state->destination_offset;
  }
  state->expected_next_sensor = NULL;
  state->expected_next_sensor_index = -1;
  state->dist_to_next_sens = 0;
  state->has_path_completed = 1;
  state->bsens_timeout_active = 0;

  update_sensor_display(state, delta_t, delta_d, current_debug_line++);
}

void _handle_accel_finished(struct movement_state *state) {
  if (state->accel.acceleration_update_time + state->accel.duration) {
    update_acceleration_and_position(state);

    // if stopped (zpeed is zero, and accel updated to uniform stopped), finish off the path and update position
    if (state->speed == 0 && state->accel.state == UNIFORM) {
      finish_path(state);
    }
  }
}
void handle_accel_finished(
  struct movement_state *state,
  struct position *position,
  int *velocity,
  int *acceleration
) {
  _handle_accel_finished(state);

  update_current_position(state, position);
  update_current_velocity(state, velocity);
  update_current_acceleration(state, acceleration);
}

void _reverse_train(struct movement_state *state) {
  tr_set_speed(state->train_num, 15);
  state->is_reversed = !state->is_reversed;
  state->position.node = state->position.node->reverse;
  state->position.offset = -1 * state->position.offset + state->calibration.train_length;
  update_sensor_display(state, 0, 0, current_debug_line++);
}
void reverse_train(struct movement_state *state, enum direction *reverse, struct position *position) {
  _reverse_train(state);
  update_current_position(state, position);
  update_current_reverse(state, reverse);
}
