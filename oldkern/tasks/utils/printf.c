/*
 * bwio.c - busy-wait I/O routines for diagnosis
 *
 * Specific to the TS-7200 ARM evaluation board
 *
 */

#include "tasks/uart_server_notifier.h"
#include "tasks/utils/printf.h"
#include "common/constants.h"
#include "common/algs.h"
#include "emul/ts7200.h"
#include "bwio/bwio.h"

#define TASK_PRINTF_DEBUG_ON 0

struct TaskPrintString {
  char string[TASK_PRINTF_BUF_LEN];
  int length;
};

void init_print_buffer(struct TaskPrintString *buf, int uart) {
  buf->length = 1;
  memzero(buf->string, TASK_PRINTF_BUF_LEN);
  buf->string[0] = (char) (UART_TRANSMIT | (uart == COM1? UART_1_COMMAND_MASK : UART_2_COMMAND_MASK));
}

int task_putc(struct TaskPrintString *buf, char c ) {
  if (buf->length >= TASK_PRINTF_BUF_LEN) {
    bwprintf(COM2, "String too long - Attempting to print to uart a string that is over %d long!", TASK_PRINTF_BUF_LEN);
    return -1;
  }

  buf->string[buf->length] = c;
  buf->length++;
  return 0;
}

int task_putstr(struct TaskPrintString *buf, char *str) {
  while( *str ) {
    if( task_putc( buf, *str ) < 0 ) return -1;
    str++;
  }
  return 0;
}

void task_putw(struct TaskPrintString *buf, int n, char fc, char *bf ) {
  char ch;
  char *p = bf;

  while( *p++ && n > 0 ) n--;
  while( n-- > 0 ) task_putc( buf, fc );
  while( ( ch = *bf++ ) ) task_putc( buf, ch );
}

void task_format(struct TaskPrintString *buf, char *fmt, va_list va ) {
  char bf[12];
  char ch, lz;
  int w;

  while ( ( ch = *(fmt++) ) ) {
    if ( ch != '%' )
      task_putc( buf, ch );
    else {
      lz = 0; w = 0;
      ch = *(fmt++);
      switch ( ch ) {
      case '0':
        lz = 1; ch = *(fmt++);
        break;
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        ch = a2i(ch, &fmt, 10, &w );
        break;
      }
      switch( ch ) {
      case 0:
        return;
      case 'c':
        task_putc(buf, va_arg( va, char ) );
        break;
      case 's':
        task_putw(buf, w, 0, va_arg( va, char* ) );
        break;
      case 'u':
        ui2a( va_arg( va, unsigned int ), 10, bf );
        task_putw(buf, w, lz, bf );
        break;
      case 'd':
        i2a( va_arg( va, int ), bf );
        task_putw(buf, w, lz, bf );
        break;
      case 'x':
        ui2a( va_arg( va, unsigned int ), 16, bf );
        task_putw(buf, w, lz, bf );
        break;
      case '%':
        task_putc(buf, ch );
        break;
      }
    }
  }
}

int task_printf(int tid, int uart, char *fmt, ... ) {
  va_list va;
  va_start(va,fmt);

  struct TaskPrintString buf;
  init_print_buffer(&buf, uart);
  task_format(&buf, fmt, va);

  va_end(va);

  return PutStr(tid, uart, buf.string, buf.length);
}

void task_printbuf(struct TaskPrintString *buf, char *fmt, ... ) {
  va_list va;
  va_start(va,fmt);

  task_format(buf, fmt, va);

  va_end(va);
}

int dtask_printf(int tid, int uart, char *fmt, ... ) {
  if (TASK_PRINTF_DEBUG_ON) {
    va_list va;
    va_start(va,fmt);

    struct TaskPrintString buf;
    init_print_buffer(&buf, uart);
    task_format(&buf, fmt, va);

    va_end(va);

    return PutStr(tid, uart, buf.string, buf.length);
  }
  return 0;
}

void task_printf_clear_screen(struct TaskPrintString *buf) {
  task_printbuf(buf, "\033[2J");
}

void task_printf_save_cursor(struct TaskPrintString *buf) {
  task_printbuf(buf, "\033[s");
}

void task_printf_move_cursor(struct TaskPrintString *buf, int x, int y) {
  task_printbuf(buf, "\033[%d;%dH", x, y);
}

void task_printf_clear_rest_of_line(struct TaskPrintString *buf) {
  task_printbuf(buf, "\033[K");
}

void task_printf_restore_cursor(struct TaskPrintString *buf) {
  task_printbuf(buf, "\033[u");
}

int task_init_layout_and_cursor(int tid) {
  struct TaskPrintString buf;
  struct TaskPrintString buf_switches;
  init_print_buffer(&buf, COM2);
  init_print_buffer(&buf_switches, COM2);

  task_printf_clear_screen(&buf_switches);
  task_printf_move_cursor(&buf_switches, DISP_SWITCH_POSITIONS_LINE + 1, 1);
  task_printbuf(&buf_switches, "001 C   002 C   003 C   004 C   005 C   006 C");
  task_printf_move_cursor(&buf_switches, DISP_SWITCH_POSITIONS_LINE + 2, 1);
  task_printbuf(&buf_switches, "007 C   008 C   009 C   010 C   011 C   012 C");
  task_printf_move_cursor(&buf_switches, DISP_SWITCH_POSITIONS_LINE + 3, 1);
  task_printbuf(&buf_switches, "013 C   014 C   015 C   016 C   017 C   018 C");
  task_printf_move_cursor(&buf_switches, DISP_SWITCH_POSITIONS_LINE + 4, 1);
  task_printbuf(&buf_switches, "153 C   154 S   155 C   156 S");

  task_printf_move_cursor(&buf, DISP_TIME_LINE, 1);
  task_printbuf(&buf, "TIME:");

  task_printf_move_cursor(&buf, DISP_IDLE_LINE, 1);
  task_printbuf(&buf, "IDLE:");

  task_printf_move_cursor(&buf, DISP_SENSORS_LINE, 1);
  task_printbuf(&buf, "LAST SENSORS:");

  task_printf_move_cursor(&buf, DISP_ACTIVE_TRAINS_LINE, 1);
  task_printbuf(&buf, "TRAINS:");

  task_printf_move_cursor(&buf, DISP_SWITCH_POSITIONS_LINE, 1);
  task_printbuf(&buf, "----- SWITCHES -----");

  task_printf_move_cursor(&buf, DISP_LAST_COMMAND_LINE, 1);
  task_printbuf(&buf, "LAST COMMAND:");

  task_printf_move_cursor(&buf, DISP_ERROR_LINE, 1);
  task_printbuf(&buf, "ERROR:");

  task_printf_move_cursor(&buf, DISP_INPUT_LINE, 1);
  task_printbuf(&buf, "> INITIALIZING...");

  int retval = PutStr(tid, COM2, buf_switches.string, buf_switches.length);
  if (retval < 0) {
    return retval;
  } else {
    return PutStr(tid, COM2, buf.string, buf.length);
  }
}

// these functions should always save and reset
// implicit UART2/COM2
int task_print_timer(int tid, int timer_ticks) {
  struct TaskPrintString buf;
  init_print_buffer(&buf, COM2);
  task_printf_save_cursor(&buf);
  task_printf_move_cursor(&buf, DISP_TIME_LINE, 7);
  task_printf_clear_rest_of_line(&buf);

  int hours = timer_ticks / (100 * 60 * 60);
  int minutes = (timer_ticks / (100 * 60)) % 60;
  int seconds = (timer_ticks / 100) % 60;
  int deciseconds = (timer_ticks / 10 ) % 10;
  if (hours > 0) {
    task_printbuf(&buf, "%d:", hours);
  }

  if (minutes < 10) {
    task_putc(&buf, '0');
  }
  task_printbuf(&buf, "%d:", minutes);

  if (seconds < 10) {
    task_putc(&buf, '0');
  }
  task_printbuf(&buf, "%d.", seconds);
  task_printbuf(&buf, "%d", deciseconds);
  task_printf_restore_cursor(&buf);
  return PutStr(tid, COM2, buf.string, buf.length);
}

// ints are 0 <= i <= 10000
int task_print_idle(int tid, int cumul, int last_interval, int min) {
  struct TaskPrintString buf;
  init_print_buffer(&buf, COM2);
  int major, minor;

  task_printf_save_cursor(&buf);
  task_printf_move_cursor(&buf, DISP_IDLE_LINE, 1);
  task_printf_clear_rest_of_line(&buf);

  // print IDLE
  major = cumul / 100;
  minor = cumul % 100;
  task_printbuf(&buf, "IDLE: ", major);
  if (major < 10) {
    task_putc(&buf, '0');
  }
  task_printbuf(&buf, "%d.", major);
  if (minor < 10) {
    task_putc(&buf, '0');
  }
  task_printbuf(&buf, "%d%%   ", minor);

  // print 500MS
  major = last_interval / 100;
  minor = last_interval % 100;
  task_printbuf(&buf, "500MS: ", major);
  if (major < 10) {
    task_putc(&buf, '0');
  }
  task_printbuf(&buf, "%d.", major);
  if (minor < 10) {
    task_putc(&buf, '0');
  }
  task_printbuf(&buf, "%d%%   ", minor);

  // print MIN
  major = min / 100;
  minor = min % 100;
  task_printbuf(&buf, "MIN: ", major);
  if (major < 10) {
    task_putc(&buf, '0');
  }
  task_printbuf(&buf, "%d.", major);
  if (minor < 10) {
    task_putc(&buf, '0');
  }
  task_printbuf(&buf, "%d%%   ", minor);

  task_printf_restore_cursor(&buf);
  return PutStr(tid, COM2, buf.string, buf.length);
}

// sensors is array of n sensors in order
int task_print_sensors(int tid, struct ByteBuffer *last_sensors) {
  struct TaskPrintString buf;
  init_print_buffer(&buf, COM2);
  task_printf_save_cursor(&buf);
  task_printf_move_cursor(&buf, DISP_SENSORS_LINE, 14);
  task_printf_clear_rest_of_line(&buf);

  int sensors_it;
  int sensor_id;
  char sensor_letter;
  int sensor_number;
  for (
    sensors_it = (last_sensors->tail + last_sensors->len - 1) % last_sensors->len;
    sensors_it != (last_sensors->head + last_sensors->len - 1) % last_sensors->len;
    sensors_it = ((sensors_it + last_sensors->len - 1) % last_sensors->len)
  ) {
    sensor_id = (int) last_sensors->buffer[sensors_it];
    sensor_letter = 'A' + (sensor_id / 0x10);
    sensor_number = (sensor_id % 0x10) + 1;
    task_printbuf(&buf, " %c%d", sensor_letter, sensor_number);
  }

  task_printf_restore_cursor(&buf);
  return PutStr(tid, COM2, buf.string, buf.length);
}

// train speed is array of 128 num -> speed mapping
int task_print_trains(int tid, char train_speed[]) {
  struct TaskPrintString buf;
  init_print_buffer(&buf, COM2);
  task_printf_save_cursor(&buf);
  task_printf_move_cursor(&buf, DISP_ACTIVE_TRAINS_LINE, 8);
  task_printf_clear_rest_of_line(&buf);

  int it = 0;
  for (it = 0; it < NUM_TRAINS_MAX; it++) {
    if (train_speed[it] != 0) {
      task_printbuf(&buf, " %d-%d", it, train_speed[it]);
    }
  }

  task_printf_restore_cursor(&buf);
  return PutStr(tid, COM2, buf.string, buf.length);
}

// switch index between 0-21
int task_print_switch(int tid, int switch_num, char switch_dir) {
  int switch_index = 0;
  if (switch_num >= 1 && switch_num <= 18) {
    switch_index = switch_num - 1;
  } else if (switch_num >= 153 && switch_num <= 156) {
    switch_index = switch_num - 135;
  } else {
    return -1;
  }

  struct TaskPrintString buf;
  init_print_buffer(&buf, COM2);
  task_printf_save_cursor(&buf);
  task_printf_move_cursor(&buf,
    1 + DISP_SWITCH_POSITIONS_LINE + (switch_index / 6),
    5 + (switch_index % 6) * 8
  );

  task_putc(&buf, switch_dir);

  task_printf_restore_cursor(&buf);
  return PutStr(tid, COM2, buf.string, buf.length);
}

// str is normal str
int task_print_last_command(int tid, char *fmt, ...) {
  struct TaskPrintString buf;
  init_print_buffer(&buf, COM2);
  task_printf_save_cursor(&buf);
  task_printf_move_cursor(&buf, DISP_LAST_COMMAND_LINE, 15);
  task_printf_clear_rest_of_line(&buf);

  va_list va;
  va_start(va,fmt);

  task_format(&buf, fmt, va);

  va_end(va);

  task_printf_restore_cursor(&buf);
  return PutStr(tid, COM2, buf.string, buf.length);
}

// normal printf
int task_print_error(int tid, char *fmt, ... ) {
  struct TaskPrintString buf;
  init_print_buffer(&buf, COM2);
  task_printf_save_cursor(&buf);
  task_printf_move_cursor(&buf, DISP_ERROR_LINE, 8);
  task_printf_clear_rest_of_line(&buf);

  va_list va;
  va_start(va,fmt);

  task_format(&buf, fmt, va);

  va_end(va);

  task_printf_restore_cursor(&buf);
  return PutStr(tid, COM2, buf.string, buf.length);
}

// switch index between 0-21
int task_print_input_backspace(int tid, int length) {
  struct TaskPrintString buf;
  init_print_buffer(&buf, COM2);

  task_printf_move_cursor(&buf, DISP_INPUT_LINE, 3 + length);
  task_printf_clear_rest_of_line(&buf);

  return PutStr(tid, COM2, buf.string, buf.length);
}

// wrapper
int task_print_input(int tid, char c) {
  return Putc(tid, COM2, c);
}

int task_clear_input_line(int tid) {
  struct TaskPrintString buf;
  init_print_buffer(&buf, COM2);

  task_printf_move_cursor(&buf, DISP_INPUT_LINE, 3);
  task_printf_clear_rest_of_line(&buf);

  return PutStr(tid, COM2, buf.string, buf.length);
}


int uart1_init_fifo(int state) {
  volatile int *line, buf;
  line = (int *)( UART1_BASE + UART_LCRH_OFFSET );
  buf = *line;
  buf = state ? buf | FEN_MASK : buf & ~FEN_MASK;

  *line = buf;
  return 0;
}

int uart2_init_fifo(int state) {
  volatile int *line, buf;
  line = (int *)( UART2_BASE + UART_LCRH_OFFSET );
  buf = *line;
  buf = state ? buf | FEN_MASK : buf & ~FEN_MASK;

  *line = buf;
  return 0;
}

int uart1_set_speed() {
  volatile int *high, *mid, *low;
  high = (int *)( UART1_BASE + UART_LCRH_OFFSET );
  mid = (int *)( UART1_BASE + UART_LCRM_OFFSET );
  low = (int *)( UART1_BASE + UART_LCRL_OFFSET );
  *mid = 0x0;
  *low = 0xbf;
  *high = *high;
  return 0;
}

int uart2_set_speed() {
  volatile int *high, *mid, *low;
  high = (int *)( UART2_BASE + UART_LCRH_OFFSET );
  mid = (int *)( UART2_BASE + UART_LCRM_OFFSET );
  low = (int *)( UART2_BASE + UART_LCRL_OFFSET );
  *mid = 0x0;
  *low = 0x3;
  *high = *high;
  return 0;
}



int train_set_train_speed(int tid, unsigned int train_number, unsigned int train_speed) {
  struct TaskPrintString buf;
  init_print_buffer(&buf, COM1);

  task_putc(&buf, (char) train_speed);
  task_putc(&buf, (char) train_number);

  return PutStr(tid, COM1, buf.string, buf.length);
}

int train_reverse_train(int tid, unsigned int train_number) {
  return train_set_train_speed(tid, train_number, 15);
}

int train_set_switch_dir(int tid, unsigned int switch_number, char switch_dir) {
  struct TaskPrintString buf;
  init_print_buffer(&buf, COM1);

  if (switch_dir == 'C' || switch_dir == 'c') {
    task_putc(&buf, (char) 34);
  } else if (switch_dir == 'S' || switch_dir == 's') {
    task_putc(&buf, (char) 33);
  } else {
    return -12;
  }
  task_putc(&buf, (char) switch_number);

  return PutStr(tid, COM1, buf.string, buf.length);
}

int train_solenoid_off(int tid) {
  return Putc(tid, COM1, (char) 32);
}

int train_initiate_dump(int tid, unsigned int num_modules) {
  if (num_modules <= 0) {
    return -1;
  }
  return Putc(tid, COM1, (char) (128 + num_modules));
}

int train_set_reset_mode(int tid, int reset_mode) {
  char buf = 0;
  if (reset_mode == OFF) {
    buf = (char) 128;
  } else if (reset_mode == ON) {
    buf = (char) 192;
  } else {
    return -1;
  }
  return Putc(tid, COM1, buf);
}

