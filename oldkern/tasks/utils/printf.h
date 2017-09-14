#ifndef _TASKS__UTILS__PRINTF_H_
#define _TASKS__UTILS__PRINTF_H_

#include "common/byte_buffer.h"

#define TASK_PRINTF_DEBUG_ON 0
#define TASK_PRINTF_BUF_LEN 256

#define DISP_TIME_LINE 1
#define DISP_IDLE_LINE (1 + DISP_TIME_LINE)
#define DISP_SENSORS_LINE (1 + DISP_IDLE_LINE)
#define DISP_ACTIVE_TRAINS_LINE (1 + DISP_SENSORS_LINE)
#define DISP_SWITCH_POSITIONS_LINE (2 + DISP_ACTIVE_TRAINS_LINE)
#define DISP_LAST_COMMAND_LINE (6 + DISP_SWITCH_POSITIONS_LINE)
#define DISP_INPUT_LINE (1 + DISP_LAST_COMMAND_LINE)
#define DISP_ERROR_LINE (1 + DISP_INPUT_LINE)

int dtask_printf(int tid, int uart, char *fmt, ... );
int task_printf(int tid, int uart, char *fmt, ... );

int task_init_layout_and_cursor(int tid);

// these functions should always save and reset
// implicit UART2/COM2
int task_print_timer(int tid, int timer_ticks);

// ints are 0 <= i <= 10000
int task_print_idle(int tid, int cumul, int last100ms, int min);

// sensors is array of n sensors in order
int task_print_sensors(int tid, struct ByteBuffer *last_sensors);

// train speed is array of 128 num -> speed mapping
int task_print_trains(int tid, char train_speed[]);

// switch index between 0-21
int task_print_switch(int tid, int switch_index, char switch_dir);

// str is normal str
int task_print_last_command(int tid, char *fmt, ...);

// normal printf
int task_print_error(int tid, char *fmt, ... );

// backspace input line
int task_print_input_backspace(int tid, int length);

// clear input line
int task_clear_input_line(int tid);

// input
int task_print_input(int tid, char c);


int uart1_init_fifo(int state);
int uart2_init_fifo(int state);
int uart1_set_speed();
int uart2_set_speed();

int train_set_train_speed(int tid, unsigned int train_number, unsigned int train_speed);
int train_reverse_train(int tid, unsigned int train_number);
int train_set_switch_dir(int tid, unsigned int switch_number, char switch_dir);
int train_solenoid_off(int tid);
int train_initiate_dump(int tid, unsigned int num_modules);
int train_set_reset_mode(int tid, int reset_mode);

#endif
