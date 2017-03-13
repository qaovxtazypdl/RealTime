#include <common/syscall.h>
#include <track_node.h>
#include <train.h>
#include <routing.h>
#include <sensors.h>
#include <clock_server.h>


static void update_path(struct track_node **path, struct track_node **new_path) {
  int i;
  struct track_node **c;

  for (i = 0, c = new_path; *c != NULL; c++, i++)
    path[i] = *c;

  path[i] = NULL;
  path_activate(path);
}

static void update_velocity(struct track_node **sensors, int *velocity, int last_update) {
  //TODO implement
  *velocity = *velocity;
}

static void update_position(struct track_node **sensors, int *velocity, int last_update) {
  //TODO implement.
}

static void update_stopping_time(struct track_node **sensors,
                                 int *stopping_time,
                                 int velocity,
                                 struct position position,
                                 struct track_node **path) {
}

void train() {
  int td;
  int num, i;
  int velocity;
  int direction;
  int last_update;
  int stopping_time;
  struct track_node **c;

  struct position position;
  struct position destination;
  struct track_node *path[MAX_PATH_LEN];

  union {
    struct track_node *sensors[NUM_SENSORS];
    struct train_command command;
  } msg;

  int sens_td = sensor_subscribe();
  int delay_td = -1;
  
  last_update = get_time();
  while(1) {
    receive(&td, &msg, sizeof(msg));

    if(td == sens_td) {
      update_velocity(msg.sensors, &velocity, last_update);
      update_position(msg.sensors, &position, last_update);
      update_stopping_time(msg.sensors, &stopping_time, velocity, position, struct track_node **path);
      last_update = get_time();
      delay_td = delay_async(async, time);
    } else if(td == delay_td) {
      //CHECK stopping time against current time and stop if equal.
    } else {
      switch(msg.command.type) {
        case TRAIN_COMMAND_SET_PATH:
          update_path(path, msg.command.path);
          break;
        default:
          break;
      }
    }
    
  }
}
