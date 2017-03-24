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
#include <track_node.h>
#include <switch.h>

static int current_debug_line = 40;

#define DURATION_FOREVER 0x0fffffff
#define SENSOR_ATTRIBUTION_DISTANCE_TOLERANCE 500

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

/* Train state was put on the stack of the train task for a reason.  I
   should be able to tell which aspect of train state each funciton
   call modifies from train() (and it should be as small as possible).
   Massive structs like this are generally a bad idea unless they
   represent something which is truly opaque and manipulated
   atomically.  */

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
  int path_length;
  int path_index;
  int destination_offset;

  // stopping
  int stopping_time;
  struct position stop_position;
  int stop_delay_tid;
  int update_delay_tid;
  enum travel_method travel_method;

  // expectation/sensor attribution
  struct track_node *expected_next_sensor;
  int expected_next_sensor_index;
  int dist_to_next_sens;
  enum velocity_state accel_from_last_sensor;
};

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

inline void update_current_quantities(
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

/* Make all of the functions below which are not part of the API static.  */
void print_broken(char *name) {
  printf(COM2,
    "%s%m%sBROKEN: %s\n\r%s",
    SAVE_CURSOR,
    (int[]){0, current_debug_line++},
    CLEAR_LINE,
    name,
    RESTORE_CURSOR
  );
}

void update_position_display(struct movement_state *state) {
  printf(COM2,
    "%s%m%strain %d position %d mm past sensor %d\n\r%s",
    SAVE_CURSOR,
    /* Avoid magic numbers, define this as a macro and move this function into console.c */
    (int[]){0, 39},
    CLEAR_LINE,
    state->train_num,
    state->position.offset, state->position.node->num,
    RESTORE_CURSOR
  );
}

void update_sensor_display(struct movement_state *state, int delta_t, int delta_d) {
  char *accel_state;
  if (state->accel_from_last_sensor == ACCELERATING) {
    accel_state = "ACCEL";
  } else if (state->accel_from_last_sensor == DECELERATING) {
    accel_state = "DECEL";
  } else {
    accel_state = "";
  }

  printf(COM2,
    "%s%m%sTRAIN{%d}\tsens: %s\tdel_t: %d\tdel_d: %d mm\texp_next: %s\t%s%s",
    SAVE_CURSOR,
    (int[]){0, current_debug_line++},
    CLEAR_LINE,
    state->train_num, state->position.node ? state->position.node->name : "Y",
    delta_t, delta_d,
    state->expected_next_sensor ? state->expected_next_sensor->name : "X",
    accel_state,
    RESTORE_CURSOR
  );
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

  if (state->has_path_completed || (state->accel.state == UNIFORM && state->speed == 0)) {
    // train has already stopped moving - path is stale
    new_position->node = state->position.node;
    new_position->offset = state->position.offset;
  } else {
    // train moving -> path is non stale
    int new_offset = 0;
    int stop_index = advance_train_by_sensor(state->path, state->path_index, &new_offset, d + state->position.offset);
    new_position->node = state->path[stop_index];
    new_position->offset = new_offset;
  }
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
/* Consumes */
void update_speed(struct movement_state *state, int speed, int until) {
  if (speed != state->speed) {
    int current_time = get_time();
    if (state->speed == 0 && speed > 0) {
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
void update_speed_wrap(
  struct movement_state *state,
  int speed,
  int until,
  struct position *position,
  int *velocity,
  int *acceleration
) {
  update_speed(state, speed, until);

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

    update_speed(state, 12, 0);

    int stopping_distance =
      (state->is_reversed ? state->calibration.reverse_offset : state->calibration.forward_offset) +
      (state->calibration.stopping_distance[state->speed]);

    if (distance < stopping_distance) {
      return -1;
    }
    int distance_before_stop = distance - stopping_distance;

    int stop_offset = 0;
    int stop_index = advance_train_by_sensor(state->path, state->path_index, &stop_offset, distance_before_stop + state->position.offset);
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

  if (state->expected_next_sensor == NULL) {
    state->dist_to_next_sens = 0;
    return -1;
  } else {
    state->dist_to_next_sens = distance;
    return state->expected_next_sensor->num;
  }
}

void update_path(struct movement_state *state, struct track_node **new_path, int posn_offset, int dest_offset) {
  int i;
  struct track_node **c;
  for (i = 0, c = new_path; *c != NULL; c++, i++) {
    state->path[i] = *c;
  }
  state->path[i] = NULL;
  current_debug_line = 40;

  // if source node has changed from current position,
  state->position.node = state->path[0];
  state->position.offset = posn_offset;
  state->path_index = 0;
  state->path_length = i;
  state->destination_offset = dest_offset;
  state->has_path_completed = 0;

  update_travel_plans(state);
  path_activate(state->path);
  update_sensor_prediction(state);

  update_sensor_display(state, 0, 0);
}
void update_path_wrap(
  struct movement_state *state,
  struct track_node **new_path,
  int posn_offset,
  int dest_offset,
  struct position *position,
  int *velocity,
  int *acceleration,
  struct track_node **path
) {
  update_path(state, new_path, posn_offset, dest_offset);

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
void _get_broken_switch_next_sensors(
  struct track_node *node,
  struct possible_broken_node broken[6],
  int *length,
  int d,
  struct track_node *last_branch,
  int last_branch_direction
) {
  switch(node->type) {
    case NODE_BRANCH:
      _get_broken_switch_next_sensors(node->edge[DIR_STRAIGHT].dest, broken, length, d + node->edge[DIR_STRAIGHT].dist, node, DIR_STRAIGHT);
      _get_broken_switch_next_sensors(node->edge[DIR_CURVED].dest, broken, length, d + node->edge[DIR_CURVED].dist, node, DIR_CURVED);
      break;
    case NODE_MERGE:
    case NODE_ENTER:
      _get_broken_switch_next_sensors(node->edge[DIR_AHEAD].dest, broken, length, d + node->edge[DIR_AHEAD].dist, last_branch, last_branch_direction);
      break;
    case NODE_SENSOR:
      if (last_branch != NULL) {
        broken[*length].sensor = node;
        broken[*length].broken = last_branch;
        broken[*length].distance = d;
        broken[*length].switch_direction = last_branch_direction;
        *length = *length + 1;
      }
      break;
    case NODE_EXIT:
    case NODE_NONE:
      break;
  }
}

// skipped a sensor
// switch broken (with uncertainty in future switches)
// normal expected next sensor
// returns number of sensors gotten
int get_broken_switch_next_sensors(struct track_node *node, struct possible_broken_node broken[6]) {
  int length = 0;
  switch(node->type) {
    case NODE_BRANCH:
      _get_broken_switch_next_sensors(node->edge[DIR_STRAIGHT].dest, broken, &length, node->edge[DIR_STRAIGHT].dist, node, DIR_STRAIGHT);
      _get_broken_switch_next_sensors(node->edge[DIR_CURVED].dest, broken, &length, node->edge[DIR_CURVED].dist, node, DIR_CURVED);
      break;
    case NODE_MERGE:
    case NODE_ENTER:
    case NODE_SENSOR:
      _get_broken_switch_next_sensors(node->edge[DIR_AHEAD].dest, broken, &length, node->edge[DIR_AHEAD].dist, NULL, DIR_STRAIGHT);
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

void handle_sensors(struct movement_state *state, struct track_node **sensors) {
  int i = 0;
  if (sensors[0] == NULL) {
    return;
  }

  struct possible_broken_node broken_switch[6];
  struct possible_broken_node broken_sensor;

  get_broken_sensor_next_sensor(state->path, state->path_index, &broken_sensor);
  int bsw_sens_count = get_broken_switch_next_sensors(state->position.node, broken_switch);

/*
  printf(COM2, "bsens: %s, bsensdist: %d\n\r",  broken_sensor.sensor == NULL ? "x" : broken_sensor.sensor->name, broken_sensor.distance);
  int dbg = 0;
  for (dbg = 0; dbg < bsw_sens_count; dbg++) {
    printf(COM2, "bsw_i=%d, bsw: %s, bsw_d: %d\n\r", dbg, broken_switch[dbg].sensor->name, broken_switch[dbg].distance);
  }
*/
  int d_offset_from_current_node = get_offset_from_current_position(state) + state->position.offset;
  int distance_to_attributed_sensor = 0;
  int attributed_sensor_index = 0;
  struct track_node *attributed_sensor = NULL;
  int sensor_attribute_offset = 0;

  while (sensors != NULL && sensors[i] != NULL) {
    if (sensors[i] == state->expected_next_sensor) {
      if (abs(d_offset_from_current_node - state->dist_to_next_sens) < SENSOR_ATTRIBUTION_DISTANCE_TOLERANCE) {
        attributed_sensor = sensors[i];
        attributed_sensor_index = state->expected_next_sensor_index;
        distance_to_attributed_sensor = state->dist_to_next_sens;
      }
    } else if (sensors[i] == broken_sensor.sensor) {
      if (abs(d_offset_from_current_node - broken_sensor.distance) < SENSOR_ATTRIBUTION_DISTANCE_TOLERANCE) {
        // expected sensor was broken!
        attributed_sensor = sensors[i];
        attributed_sensor_index = broken_sensor.index;
        distance_to_attributed_sensor = broken_sensor.distance;

        sensor_register_broken(broken_sensor.broken);
        print_broken(broken_sensor.broken->name);
      }
    } else {
      // check for broken switch and determine exactly which switch was broken
      int broken_it;
      for (broken_it = 0; broken_it < bsw_sens_count; broken_it++) {
        if (
          sensors[i] == broken_switch[broken_it].sensor &&
          abs(d_offset_from_current_node - broken_switch[broken_it].distance) < SENSOR_ATTRIBUTION_DISTANCE_TOLERANCE
        ) {
          attributed_sensor = sensors[i];
          attributed_sensor_index = 0;
          distance_to_attributed_sensor = broken_switch[broken_it].distance;

// TODO: jzhao - HACK - replace this with nonblocking
          int len;
          struct track_node *srcn = attributed_sensor;
          struct track_node *dstn = state->path[state->path_length - 1];
          struct track_node *path[MAX_PATH_LEN];
          len = path_find(srcn, dstn, path);
          update_path(state, path, sensor_attribute_offset, state->destination_offset);

          struct broken_switch broken_updater;
          broken_updater.node = broken_switch[broken_it].broken;
          broken_updater.position = broken_switch[broken_it].switch_direction;

          switch_register_broken(&broken_updater);
          print_broken(broken_switch[broken_it].broken->name);
          break;
        }
      }
    }

    if (attributed_sensor != NULL) {
      // updates offset to current time offset from previous sensor.
      update_acceleration_and_position(state);
      int delta_d = d_offset_from_current_node - distance_to_attributed_sensor;
      int delta_t = delta_d * 100 / state->accel.start_velocity;

      // corrects the offset to 0 to the current sensor
      state->position.node = attributed_sensor;
      state->position.offset = sensor_attribute_offset;
      state->path_index = attributed_sensor_index;

      update_sensor_prediction(state);
      update_sensor_display(state, delta_t, delta_d);

      state->accel_from_last_sensor = state->accel.state;

      if (
        state->travel_method == LONG_MOVE &&
/* Don't compare numbers, compare nodes directly. */
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
void handle_sensors_wrap(
  struct movement_state *state,
  struct track_node **sensors,
  struct position *position,
  int *velocity,
  int *acceleration
) {
  handle_sensors(state, sensors);

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

  state->accel.state = UNIFORM;
  state->accel.start_velocity = 0;
  state->accel.acceleration_update_time = current_time;
  state->accel.duration = DURATION_FOREVER;
  state->accel_from_last_sensor = UNIFORM;
}

void finish_path(struct movement_state *state) {
  int delta_d = state->position.offset - (state->dist_to_next_sens + state->destination_offset);
  int delta_t = delta_d * 100 / state->accel.start_velocity;
  update_sensor_display(state, delta_t, delta_d);

  state->expected_next_sensor = NULL;
  state->expected_next_sensor_index = -1;
  state->dist_to_next_sens = 0;
  state->position.node = state->path[state->path_length - 1];
  state->position.offset = state->destination_offset;
  state->has_path_completed = 1;

  current_debug_line = 40;
}

void handle_accel_finished(struct movement_state *state) {
  if (state->accel.acceleration_update_time + state->accel.duration) {
    update_acceleration_and_position(state);

    // if stopped (zpeed is zero, and accel updated to uniform stopped), finish off the path and update position
    if (state->speed == 0 && state->accel.state == UNIFORM) {
      finish_path(state);
    }
  }
}
void handle_accel_finished_wrap(
  struct movement_state *state,
  struct position *position,
  int *velocity,
  int *acceleration
) {
  handle_accel_finished(state);

  update_current_position(state, position);
  update_current_velocity(state, velocity);
  update_current_acceleration(state, acceleration);
}

void reverse_train(struct movement_state *state) {
  tr_set_speed(state->train_num, 15);
  state->is_reversed = !state->is_reversed;
  state->position.node = state->position.node->reverse;
  state->position.offset = -1 * state->position.offset + state->calibration.train_length;
  update_sensor_display(state, 0, 0);
}
void reverse_train_wrap(struct movement_state *state, enum direction *reverse, struct position *position) {
  reverse_train(state);
  update_current_position(state, position);
  update_current_reverse(state, reverse);
}

void train() {
 /* These quantities should be authoritative. */
  int tid;
  int num;

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

  union {
    struct track_node *sensors[NUM_SENSORS];
    struct train_command command;
  } msg;

  int sens_tid = sensor_subscribe();

  while(1) {
    update_position_display(&state);
    receive(&tid, &msg, sizeof(msg));
    update_current_quantities(&state, &reverse, &position, &velocity, &acceleration, path);

    if(tid == sens_tid) {
      reply(tid, NULL, 0);
      handle_sensors_wrap(&state, msg.sensors, &position, &velocity, &acceleration);
    } else if(tid == state.stop_delay_tid) {
      reply(tid, NULL, 0);
      if (state.stopping_time <= get_time()) {
        update_speed_wrap(&state, 0, 0, &position, &velocity, &acceleration);
      }
    } else if (tid == state.update_delay_tid) {
      reply(tid, NULL, 0);
      handle_accel_finished_wrap(&state, &position, &velocity, &acceleration);
    } else {
      struct position new_position;
      switch(msg.command.type) {
        case TRAIN_COMMAND_SET_PATH:
          reply(tid, NULL, 0);
          update_path_wrap(&state, msg.command.path, state.position.offset, msg.command.offset, &position, &velocity, &acceleration, path);
          break;
        case TRAIN_COMMAND_GET_POSITION:
          get_position(&state, &new_position);
          reply(tid, &new_position, sizeof(struct position));
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

