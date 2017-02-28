#ifndef _TRAIN_H_
#define _TRAIN_H_

#define TRAIN_COM COM1

void init_train();

void init_train();
void tr_set_switch(int turnout, int curved); 
void tr_request_sensor_data();
void tr_set_speed(int train, int speed);
void tr_reverse(int train); 

#endif
