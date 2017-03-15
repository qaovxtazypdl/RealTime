#include <train/calibration.h>
#define NUM_CALIBRATIONS 3

void init_calibration(struct train_calibration *calibration, int train_num) {
  if (train_num == 71) {
     /* train 71 */
    calibration->train_num = 71;
    calibration->forward_offset = 25;
    calibration->reverse_offset = 142;
    calibration->train_length = 209;

    calibration->acceleration = 134;
    calibration->deceleration = 112;
    calibration->short_move_time_offset = 21;

    calibration->speed_to_velocity[0] = 0;
    calibration->speed_to_velocity[1] = 12;
    calibration->speed_to_velocity[2] = 15;
    calibration->speed_to_velocity[3] = 17;
    calibration->speed_to_velocity[4] = 32;
    calibration->speed_to_velocity[5] = 63;
    calibration->speed_to_velocity[6] = 97;
    calibration->speed_to_velocity[7] = 136;
    calibration->speed_to_velocity[8] = 179;
    calibration->speed_to_velocity[9] = 236;
    calibration->speed_to_velocity[10] = 290;
    calibration->speed_to_velocity[11] = 349;
    calibration->speed_to_velocity[12] = 416;
    calibration->speed_to_velocity[13] = 476;
    calibration->speed_to_velocity[14] = 557;

    // offset 21
    calibration->stopping_distance[0] = 0;
    calibration->stopping_distance[1] = 6;
    calibration->stopping_distance[2] = 6;
    calibration->stopping_distance[3] = 6;
    calibration->stopping_distance[4] = 16;
    calibration->stopping_distance[5] = 36;
    calibration->stopping_distance[6] = 69;
    calibration->stopping_distance[7] = 119;
    calibration->stopping_distance[8] = 175;
    calibration->stopping_distance[9] = 271;
    calibration->stopping_distance[10] = 381;
    calibration->stopping_distance[11] = 519;
    calibration->stopping_distance[12] = 692;
    calibration->stopping_distance[13] = 882;
    calibration->stopping_distance[14] = 1170;
  } else if (train_num == 63) {
    /* train 63 */
    calibration->train_num = 63;
    calibration->forward_offset = 34;
    calibration->reverse_offset = 143;
    calibration->train_length = 208;

    calibration->acceleration = 150;
    calibration->deceleration = 125;
    calibration->short_move_time_offset = 25;

    calibration->speed_to_velocity[0] = 0;
    calibration->speed_to_velocity[1] = 10;
    calibration->speed_to_velocity[2] = 75;
    calibration->speed_to_velocity[3] = 123;
    calibration->speed_to_velocity[4] = 171;
    calibration->speed_to_velocity[5] = 226;
    calibration->speed_to_velocity[6] = 279;
    calibration->speed_to_velocity[7] = 334;
    calibration->speed_to_velocity[8] = 377;
    calibration->speed_to_velocity[9]  = 430;
    calibration->speed_to_velocity[10] = 480;
    calibration->speed_to_velocity[11] = 534;
    calibration->speed_to_velocity[12] = 583;
    calibration->speed_to_velocity[13] = 636;
    calibration->speed_to_velocity[14] = 625;

    // offset 18
    calibration->stopping_distance[0] = 0;
    calibration->stopping_distance[1] = 5;
    calibration->stopping_distance[2] = 62;
    calibration->stopping_distance[3] = 150;
    calibration->stopping_distance[4] = 225;
    calibration->stopping_distance[5] = 306;
    calibration->stopping_distance[6] = 384;
    calibration->stopping_distance[7] = 481;
    calibration->stopping_distance[8] = 538;
    calibration->stopping_distance[9] = 608;
    calibration->stopping_distance[10] = 673;
    calibration->stopping_distance[11] = 762;
    calibration->stopping_distance[12] = 858;
    calibration->stopping_distance[13] = 936;
    calibration->stopping_distance[14] = 926;
  } else {// if (train_num == 58) {
    /* train 58 */
    calibration->train_num = 58;
    calibration->forward_offset = 24;
    calibration->reverse_offset = 143;
    calibration->train_length = 208;

    calibration->acceleration = 143;
    calibration->deceleration = 127;
    calibration->short_move_time_offset = 28;

    calibration->speed_to_velocity[0] = 0;
    calibration->speed_to_velocity[1] = 12;
    calibration->speed_to_velocity[2] = 16;
    calibration->speed_to_velocity[3] = 18;
    calibration->speed_to_velocity[4] = 33;
    calibration->speed_to_velocity[5] = 64;
    calibration->speed_to_velocity[6] = 99;
    calibration->speed_to_velocity[7] = 143;
    calibration->speed_to_velocity[8] = 188;
    calibration->speed_to_velocity[9]  = 249;
    calibration->speed_to_velocity[10] = 305;
    calibration->speed_to_velocity[11] = 368;
    calibration->speed_to_velocity[12] = 437;
    calibration->speed_to_velocity[13] = 501;
    calibration->speed_to_velocity[14] = 589;

    // offset 4
    calibration->stopping_distance[0] = 0;
    calibration->stopping_distance[1] = 3;
    calibration->stopping_distance[2] = 6;
    calibration->stopping_distance[3] = 8;
    calibration->stopping_distance[4] = 19;
    calibration->stopping_distance[5] = 42;
    calibration->stopping_distance[6] = 71;
    calibration->stopping_distance[7] = 125;
    calibration->stopping_distance[8] = 190;
    calibration->stopping_distance[9] = 292;
    calibration->stopping_distance[10] = 400;
    calibration->stopping_distance[11] = 547;
    calibration->stopping_distance[12] = 739;
    calibration->stopping_distance[13] = 939;
    calibration->stopping_distance[14] = 1246;
  }
}
