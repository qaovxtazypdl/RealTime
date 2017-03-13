#ifndef _TRAIN_CALIBRATIONS_H_
#define _TRAIN_CALIBRATIONS_H_

struct train_calibration {
  int train_num;
  int speed_to_velocity[15]; // mm/s  - 14 speeds (1-14 index)
  int stopping_distance[15]; // mm

  int acceleration;
  int deceleration;
  int short_move_time_offset;

  int forward_offset;
  int reverse_offset;
  int train_length;
};

// using trains 58 63 71 - defaults to 63.
void init_calibration(struct train_calibration *calibration, int train_num);
#endif
