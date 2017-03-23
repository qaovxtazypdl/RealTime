#ifndef _SENSORS_H_
#define _SENSORS_H_

#define NUM_SENSORS 80
#define MAX_SENSOR_SIZE (NUM_SENSORS + 1)

void init_sensors();
int sensor_subscribe();

int sensor_register_broken(struct track_node *sensor);
int sensor_get_broken(struct track_node *sensors[MAX_SENSOR_SIZE]);

#endif
