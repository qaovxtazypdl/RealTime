#ifndef _SENSOR_H_
#define _SENSOR_H_

int sensor_register_broken(struct track_node *sensor);
int sensor_get_broken(struct track_node *sensors[MAX_SENSOR]);

#endif
