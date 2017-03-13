#ifndef _TRAIN_CALIBRATIONS_H_
#define _TRAIN_CALIBRATIONS_H_

struct train_calibration_t {
  int train_num;
  int speed_to_velocity[15]; // mm/s  - 14 speeds (1-14 index)
  int stopping_distance[15]; // mm

  int forward_offset;
  int reverse_offset;
  int train_length;
};

// using trains 58 63 71 - defaults to 63.
void init_calibration(struct train_calibration_t *calibration, int train_num);
#endif
