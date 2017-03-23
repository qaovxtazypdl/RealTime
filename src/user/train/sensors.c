#include <train.h>
#include <sensors.h>
#include <ns.h>
#include <clock_server.h>
#include <io.h>
#include <train_io.h>
#include <assert.h>
#include <common/syscall.h>
#include <track.h>

#define SENSOR_LINE_NUM 1
#define SENSOR_DATA_INTERVAL 10

static int relay_td = -1;
static int courier_td = -1;
static int notifier_td = -1;
static int broken_sensors_td = -1;

static int process_sensor_data(struct track_node **sensors) {
  char b;
  int i, j, num, s;

  for (i = 0, s = 0; i < 10; i++) {
    getc(TRAIN_COM, &b);

    for (j = 0; j < 8; j++) {
      if(i % 2)
        num = 16 - j;
      else
        num = 8 - j;

      if((1 << j) & b)
        sensors[s++] = g_sensors[((i/2)*16) + (num - 1)];
    }
  }

  sensors[s] = NULL;
  return s;
}


static void printer() {
  struct track_node *sensors[NUM_SENSORS + 1];
  struct track_node **s;
  char str[1024];
  int sz = 0, seen = 0;
  int td;
  int r;

  sensor_subscribe();
  while(1) {
    r = receive(&td, sensors, sizeof(sensors));
    reply(td, NULL, 0);

    sz = 0;
    sz += sprintf(str, "%s%m%s", SAVE_CURSOR, (int[]){ 0, SENSOR_LINE_NUM }, CLEAR_LINE);
    sz += sprintf(str + sz, "[ ");

    seen = 0;
    for (s = sensors; *s != NULL; s++) {
      seen++;
      sz += sprintf(str + sz, "%s | ", (*s)->name);
    }

    if(seen)
      sz -= 3;
    sz += sprintf(str + sz, " ]%s", RESTORE_CURSOR);
    putstr(COM2, str);
  }
}

static void courier() {
  struct task_node *sensors[NUM_SENSORS];
  int clients[100];
  int c = 0, i;
  int sz;
  int td;

  while(1) {
    sz = receive(&td, sensors, sizeof(sensors));
    reply(td, NULL, 0);

    if(td == notifier_td) {
      for (i = 0; i < c; i++) {
        send(clients[i], sensors, sz, NULL, 0);
      }
    } else {
      clients[c++] = td;
    }
  }
}

static void notifier() {
  int activated;
  struct track_node *sensors[80];
  while(1) {
    delay(SENSOR_DATA_INTERVAL);
    tr_request_sensor_data();
    activated = process_sensor_data(sensors);
    send(courier_td, sensors, sizeof(sensors[0]) * (activated + 1), NULL, 0);
  }
}

/* API */

/* Allows a task to subscribe to sensor updates from the notifier,
   sensor data will be returned as a NULL terminated list of
   track_node pointers. The calling task should provide a buffer of
   track_node pointers which is NUM_SENSORS long to accomodate all
   possible sensors, although fewer bytes may be returned. */

int sensor_subscribe() {
  assert(courier_td != -1, "The courier must be initialized before this function can be called.");
  send(courier_td, NULL, 0, NULL, 0);
  return courier_td;
}

/*
  Broken sensors server stuff
*/

int sensor_register_broken(struct track_node *sensor) {
  if (broken_sensors_td < 1 || sensor == NULL) return -1;
  return send(broken_sensors_td, &sensor, sizeof(struct track_node *), NULL, 0);
}

int sensor_get_broken(struct track_node *sensors[MAX_SENSOR_SIZE]) {
  if (broken_sensors_td < 1) return -1;
  struct track_node *ptr = NULL;
  return send(broken_sensors_td, &ptr, sizeof(struct track_node *), sensors, MAX_SENSOR_SIZE * sizeof(struct track_node *));
}

void broken_sensors_server() {
  struct track_node *broken_sensors[MAX_SENSOR_SIZE];
  int broken_sensor_size = 0;
  broken_sensors[0] = NULL;

  struct track_node *new_sensor_node = NULL;
  int tid = -1;
  int i;
  while(1) {
    receive(&tid, &new_sensor_node, sizeof(struct track_node *));
    if (new_sensor_node == NULL) {
      // GET
      reply(tid, broken_sensors, (broken_sensor_size + 1) * sizeof(struct track_node *));
    } else {
      // REGISTER
      int sensor_already_registered = 0;

      // check if already exists
      for (i = 0; i < broken_sensor_size; i++) {
        if (new_sensor_node == broken_sensors[i]) {
          sensor_already_registered = 1;
          break;
        }
      }

      if (!sensor_already_registered) {
        broken_sensors[broken_sensor_size] = new_sensor_node;
        broken_sensors[broken_sensor_size + 1] = NULL;
        broken_sensor_size++;
      }

      reply(tid, NULL, 0);
    }
  }
}

void init_sensors() {
  broken_sensors_td = create(1, broken_sensors_server);
  courier_td = create(0, courier);
  notifier_td = create(0, notifier);
  create(1, printer);
}
