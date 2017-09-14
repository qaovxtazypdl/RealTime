#include "tasks/uart_server_notifier.h"
#include "tasks/timer_server_notifier.h"
#include "tasks/name_server.h"
#include "tasks/utils/ns_names.h"
#include "tasks/utils/printf.h"
#include "common/constants.h"
#include "common/assert.h"
#include "common/algs.h"
#include "common/byte_buffer.h"
#include "emul/ts7200.h"
#include "bwio/bwio.h"
#include "kernel/syscall.h"
#include "common/byte_buffer.h"

#define NUM_SENSOR_MODULES 5
#define EXPECTED_DUMP_BYTES (2 * NUM_SENSOR_MODULES)

#define TRAIN_REVERSE_REQUEST_COMMAND 'r'
#define TRAIN_SPEED_REQUEST_COMMAND 's'

struct SwitchRequest {
  int switch_num;
  char switch_dir;
};
struct TrainRequest {
  char command;
  char train_num;
  char train_speed;
};
struct ReverseTrainRequest {
  char train_num;
  char restart_speed;
  int restart_time;
};

int SetSwitch(int switch_num, char switch_dir) {
  if (switch_dir != 'C' && switch_dir != 'S') {
    return -1;
  }

  int switch_controller_id = 0;
  switch_controller_id = WhoIs(SWITCH_CONTROLLER_NS_NAME);
  if (switch_controller_id < 0) {
    bwprintf(COM2, "SetSwitch - failed to whois: %d\n\r", switch_controller_id);
  }

  struct SwitchRequest request;
  request.switch_num = switch_num;
  request.switch_dir = switch_dir;

  int reply = 0;
  int retval = 0;
  retval = Send(switch_controller_id, (char *) &request, sizeof(struct SwitchRequest), (char *) &reply, 4);

  return retval < 0 ? retval : reply;
}
int SetTrainSpeed(int train_num, int train_speed) {
  if (train_num < 0 || train_num >= 128 || train_speed < 0 || train_speed >= 128) {
    return -4;
  }

  int train_controller_id = 0;
  train_controller_id = WhoIs(TRAIN_CONTROLLER_NS_NAME);
  if (train_controller_id < 0) {
    bwprintf(COM2, "SetSwitch - failed to whois: %d\n\r", train_controller_id);
  }

  struct TrainRequest request;
  request.command = TRAIN_SPEED_REQUEST_COMMAND;
  request.train_num = train_num;
  request.train_speed = train_speed;

  int reply = 0;
  int retval = 0;
  retval = Send(train_controller_id, (char *) &request, sizeof(struct TrainRequest), (char *) &reply, 4);

  return retval < 0 ? retval : reply;
}
int ReverseTrain(int train_num) {
  if (train_num < 0 || train_num >= 128) {
    return -4;
  }

  int train_controller_id = 0;
  train_controller_id = WhoIs(TRAIN_CONTROLLER_NS_NAME);
  if (train_controller_id < 0) {
    bwprintf(COM2, "SetSwitch - failed to whois: %d\n\r", train_controller_id);
  }

  struct TrainRequest request;
  request.command = TRAIN_REVERSE_REQUEST_COMMAND;
  request.train_num = train_num;

  int reply = 0;
  int retval = 0;
  retval = Send(train_controller_id, (char *) &request, sizeof(struct TrainRequest), (char *) &reply, 4);

  return retval < 0 ? retval : reply;
}

void task__timer_display() {
  int timer_server_tid = WhoIs(TIMERSERVER_NS_NAME);
  if (timer_server_tid < 0) {
    bwprintf(COM2, "Timer Display - Failed to get timer server tid: %d\n\r", timer_server_tid);
  }

  int print_server_tid = WhoIs(UART2_XMIT_SERVER_NS_NAME);
  if (print_server_tid < 0) {
    bwprintf(COM2, "Timer Display - Failed to get print server tid: %d\n\r", print_server_tid);
  }

  int retval = 0;
  int timer_ticks = 0;
  for (;;) {
    retval = Delay(timer_server_tid, 10);
    if (retval < 0) {
      bwprintf(COM2, "Timer Display - delay failed: %d.\n\r", retval);
    }

    timer_ticks = Time(timer_server_tid);
    if (timer_ticks < 0) {
      bwprintf(COM2, "Timer Display - time failed: %d.\n\r", timer_ticks);
    }

    retval = task_print_timer(print_server_tid, timer_ticks);
    if (retval < 0) {
      bwprintf(COM2, "Timer Display - time update print failed: %d.\n\r", retval);
    }
  }
}

void task__command_parser() {
  int print_server_tid = WhoIs(UART2_XMIT_SERVER_NS_NAME);
  if (print_server_tid < 0) {
    bwprintf(COM2, "Command Parser - Failed to get print server tid: %d\n\r", print_server_tid);
  }

  int receive_server_tid = WhoIs(UART2_RCV_SERVER_NS_NAME);
  if (receive_server_tid < 0) {
    bwprintf(COM2, "Command Parser - Failed to get receive server tid: %d\n\r", receive_server_tid);
  }

  int retval = 0;
  char input_char = 0;

  struct ByteBuffer command_buffer;
  char mread[1024];
  init_byte_buffer(&command_buffer, mread, 1024);

  char parse_buffer[128]; memzero(parse_buffer, 128);

  retval = task_clear_input_line(print_server_tid);
  if (retval < 0) {
    bwprintf(COM2, "Command Parser - CLEAR input failed: %d\n\r", retval);
  }

  for(;;) {
    input_char = retval = Getc(receive_server_tid, COM2);
    if (retval < 0) {
      bwprintf(COM2, "Command Parser - Getc failed: %d\n\r", retval);
    }

    // first echo the char
    retval = task_print_input(print_server_tid, input_char);
    if (retval < 0) {
      bwprintf(COM2, "Command Parser - Echo failed: %d\n\r", retval);
    }

    if (input_char == (char) 8) {
      // backspace
      byte_buffer_pop_end(&command_buffer);

      retval = task_print_input_backspace(print_server_tid, byte_buffer_count(&command_buffer));
      if (retval < 0) {
        bwprintf(COM2, "Command Parser - BS failed: %d\n\r", retval);
      }
    } else if (input_char == (char) 13) {
      retval = task_print_error(print_server_tid, "");
      if (retval < 0) {
        bwprintf(COM2, "Command Parser - error clear failed: %d\n\r", retval);
      }

      // parse command (CR)
      parse_next_token(&command_buffer, parse_buffer);
      if (strcmp(parse_buffer, "q") == 0) {
        Shutdown();
        break;
      } else if (strcmp(parse_buffer, "tr") == 0) {
        // tr trainnum trainspeed
        unsigned int train_num;
        unsigned int train_speed;

        parse_next_token(&command_buffer, parse_buffer);
        str2uint(parse_buffer, &train_num);
        parse_next_token(&command_buffer, parse_buffer);
        str2uint(parse_buffer, &train_speed);

        if (train_num < 0 || train_num >= 128 || train_speed < 0 || train_speed >= 128) {
          retval = task_print_error(print_server_tid, "Invalid train num: %d, or speed: %d", train_num, train_speed);
          if (retval < 0) {
            bwprintf(COM2, "Command Parser - error print failed: %d\n\r", retval);
          }
        } else {
          retval = SetTrainSpeed(train_num, train_speed);
          if (retval < 0) {
            bwprintf(COM2, "Command Parser - set train speed failed: %d\n\r", retval);
          }

          retval = task_print_last_command(print_server_tid, "%s %d %d", "tr", train_num, train_speed);
          if (retval < 0) {
            bwprintf(COM2, "Command Parser - last command print failed: %d\n\r", retval);
          }
        }
      } else if (strcmp(parse_buffer, "rv") == 0) {
        // rv trainnum
        unsigned int train_num;

        parse_next_token(&command_buffer, parse_buffer);
        str2uint(parse_buffer, &train_num);

        if (train_num < 0 || train_num >= 128) {
          retval = task_print_error(print_server_tid, "Invalid train num: %d.", train_num);
          if (retval < 0) {
            bwprintf(COM2, "Command Parser - error print failed: %d\n\r", retval);
          }
        } else {
          retval = ReverseTrain(train_num);
          if (retval < 0) {
            bwprintf(COM2, "Command Parser - reversing train failed: %d\n\r", retval);
          }

          retval = task_print_last_command(print_server_tid, "%s %d", "rv", train_num);
          if (retval < 0) {
            bwprintf(COM2, "Command Parser - last command print failed: %d\n\r", retval);
          }
        }
      } else if (strcmp(parse_buffer, "sw") == 0) {
        // sw number direction
        unsigned int switch_num;
        char direction;

        parse_next_token(&command_buffer, parse_buffer);
        str2uint(parse_buffer, &switch_num);
        parse_next_token(&command_buffer, parse_buffer);
        if (strcmp(parse_buffer, "C") == 0 || strcmp(parse_buffer, "c") == 0) {
          direction = 'C';
        } else if (strcmp(parse_buffer, "s") == 0 || strcmp(parse_buffer, "S") == 0) {
          direction = 'S';
        } else {
          direction = '\0';
        }

        if (direction != '\0' && ((switch_num >= 1 && switch_num <= 18) || (switch_num >= 153 && switch_num <= 156))) {
          retval = SetSwitch(switch_num, direction);
          if (retval < 0) {
            bwprintf(COM2, "Command Parser - set switch failed: %d\n\r", retval);
          }

          retval = task_print_last_command(print_server_tid, "%s %d %c", "sw", switch_num, direction);
          if (retval < 0) {
            bwprintf(COM2, "Command Parser - last command print failed: %d\n\r", retval);
          }
        } else {
          retval = task_print_error(print_server_tid, "Invalid switch params.");
          if (retval < 0) {
            bwprintf(COM2, "Command Parser - error print failed: %d\n\r", retval);
          }
        }
      } else {
        // invalid command
        retval = task_print_error(print_server_tid, "Invalid command.");
        if (retval < 0) {
          bwprintf(COM2, "Command Parser - error print failed: %d\n\r", retval);
        }
      }

      byte_buffer_clear(&command_buffer);
      task_clear_input_line(print_server_tid);
    } else {
      byte_buffer_push(&command_buffer, input_char);
    }
  }
  Exit();
}


void task__switch_controller() {
  int retval = 0;
  retval = RegisterAs(SWITCH_CONTROLLER_NS_NAME);
  if (retval < 0) {
    bwprintf(COM2, "switch controller - failed to register: %d\n\r", retval);
  }

  int print_server_tid = WhoIs(UART2_XMIT_SERVER_NS_NAME);
  if (print_server_tid < 0) {
    bwprintf(COM2, "switch controller - Failed to get print server tid: %d\n\r", print_server_tid);
  }

  int train_xmit_server_id = 0;
  train_xmit_server_id = WhoIs(UART1_XMIT_SERVER_NS_NAME);
  if (train_xmit_server_id < 0) {
    bwprintf(COM2, "switch controller - failed to whois: %d\n\r", train_xmit_server_id);
  }

  char valid_switches[NUM_SWITCHES]; memzero(valid_switches, NUM_SWITCHES);
  char switch_state_map[NUM_SWITCHES]; memzero(switch_state_map, NUM_SWITCHES);

  unsigned int switch_iterator, switch_number;
  for (switch_iterator = 0; switch_iterator < 18; switch_iterator++) {
    valid_switches[switch_iterator] = (char) (switch_iterator + 1);
  }
  valid_switches[18] = 153;
  valid_switches[19] = 154;
  valid_switches[20] = 155;
  valid_switches[21] = 156;

  for (switch_iterator = 0; switch_iterator < NUM_SWITCHES; switch_iterator++) {
    switch_number = (unsigned int) valid_switches[switch_iterator];
    if (switch_number == 154 || switch_number == 156) {
      switch_state_map[switch_iterator] = 'S';
    } else {
      switch_state_map[switch_iterator] = 'C';
    }
  }

  struct SwitchRequest request;
  int reply = 0;
  int tid;
  int switch_index = -1;
  for (;;) {
    retval = Receive(&tid, (char *) &request, sizeof(struct SwitchRequest));
    if (retval < 0) {
      bwprintf(COM2, "switch controller - failed to receive: %d\n\r", retval);
    }

    if (
      !(request.switch_num >= 1 && request.switch_num <= 18) &&
      !(request.switch_num >= 153 && request.switch_num <= 156)
    ) {
      reply = -1;
      retval = Reply(tid, (char *) &reply, 4);
      if (retval < 0) {
        bwprintf(COM2, "switch controller - failed to reply: %d\n\r", retval);
      }
    } else {
      if (request.switch_num >= 1 && request.switch_num <= 18) {
        switch_index = request.switch_num - 1;
      } else if (request.switch_num >= 153 && request.switch_num <= 156) {
        switch_index = request.switch_num - 135;
      }
      switch_state_map[switch_index] = request.switch_dir;

      reply = retval = train_set_switch_dir(train_xmit_server_id, request.switch_num, request.switch_dir);
      if (retval < 0) {
        bwprintf(COM2, "switch controller - failed to set switch dir: %d\n\r", retval);
      }
      retval = Reply(tid, (char *) &reply, 4);
      if (retval < 0) {
        bwprintf(COM2, "switch controller - failed to reply: %d\n\r", retval);
      }

      retval = train_solenoid_off(train_xmit_server_id);
      if (retval < 0) {
        bwprintf(COM2, "switch controller - failed to turn solenoid off: %d\n\r", retval);
      }

      retval = task_print_switch(print_server_tid, request.switch_num, request.switch_dir);
      if (retval < 0) {
        bwprintf(COM2, "switch controller - failed to print switch: %d\n\r", retval);
      }
    }
  }

  Exit();
}



void task__train_reverse_queue_controller() {
  int retval = 0;

  int timer_server_tid = WhoIs(TIMERSERVER_NS_NAME);
  if (timer_server_tid < 0) {
    bwprintf(COM2, "train controller rv - Failed to get timer server tid: %d\n\r", timer_server_tid);
  }

  int train_xmit_server_tid = WhoIs(UART1_XMIT_SERVER_NS_NAME);
  if (train_xmit_server_tid < 0) {
    bwprintf(COM2, "train controller - Failed to get print server tid: %d\n\r", train_xmit_server_tid);
  }

  int reply = 0;
  int tid = 0;
  int parent_tid = MyParentTid();

  // ASSUME for now - reverse commands are constant - time delayed
  // handling in the order they come in gives chronological order
  struct ReverseTrainRequest request;
  for (;;) {
    retval = Receive(&tid, (char *) &request, sizeof(struct ReverseTrainRequest));
    if (retval < 0) {
      bwprintf(COM2, "train controller rv - failed to receive: %d\n\r", retval);
    }
    ASSERT(tid == parent_tid, "train reverse task was sent request by the wrong task: %d!", tid);

    reply = 0;
    retval = Reply(tid, (char *) &reply, 4);
    if (retval < 0) {
      bwprintf(COM2, "train controller rv - failed to reply: %d\n\r", retval);
    }

    retval = DelayUntil(timer_server_tid, request.restart_time);
    if (retval < 0) {
      bwprintf(COM2, "train controller rv - delayuntil failed: %d.\n\r", retval);
    }

    retval = train_reverse_train(train_xmit_server_tid, request.train_num);
    if (retval < 0) {
      bwprintf(COM2, "train controller rv - reverse train failed: %d\n\r", retval);
    }

    retval = train_set_train_speed(train_xmit_server_tid, request.train_num, request.restart_speed);
    if (retval < 0) {
      bwprintf(COM2, "train controller rv - train speed failed: %d\n\r", retval);
    }
  }
  Exit();
}

void task__train_controller() {
  int retval = 0;
  retval = RegisterAs(TRAIN_CONTROLLER_NS_NAME);
  if (retval < 0) {
    bwprintf(COM2, "train controller - failed to register: %d\n\r", retval);
  }

  int reverse_controller_tid = Create(9, &task__train_reverse_queue_controller);
  if (reverse_controller_tid < 0) {
    bwprintf(COM2, "train controller - failed to create child: %d\n\r", reverse_controller_tid);
  }

  int timer_server_tid = WhoIs(TIMERSERVER_NS_NAME);
  if (timer_server_tid < 0) {
    bwprintf(COM2, "train controller rv - Failed to get timer server tid: %d\n\r", timer_server_tid);
  }

  int print_server_tid = WhoIs(UART2_XMIT_SERVER_NS_NAME);
  if (print_server_tid < 0) {
    bwprintf(COM2, "train controller - Failed to get print server tid: %d\n\r", print_server_tid);
  }

  int train_xmit_server_tid = WhoIs(UART1_XMIT_SERVER_NS_NAME);
  if (train_xmit_server_tid < 0) {
    bwprintf(COM2, "train controller - Failed to get print server tid: %d\n\r", train_xmit_server_tid);
  }

  char train_speed_map[128]; memzero(train_speed_map, 128);

  int tid;
  int reply = 0;
  int child_reply = 0;
  struct TrainRequest request;
  struct ReverseTrainRequest rv_request;
  for (;;) {
    retval = Receive(&tid, (char *) &request, sizeof(struct TrainRequest));
    if (retval < 0) {
      bwprintf(COM2, "train controller - failed to receive: %d\n\r", retval);
    }

    if (request.train_num >= 128) {
      reply = -6;
      retval = Reply(tid, (char *) &reply, 4);
      if (retval < 0) {
        bwprintf(COM2, "train controller - failed to reply: %d\n\r", retval);
      }
    } else if (request.command == TRAIN_SPEED_REQUEST_COMMAND) {
      if (request.train_speed >= 128) {
        reply = -7;
        retval = Reply(tid, (char *) &reply, 4);
        if (retval < 0) {
          bwprintf(COM2, "train controller - failed to reply: %d\n\r", retval);
        }
      } else {
        train_speed_map[(int) request.train_num] = request.train_speed;
        reply = retval = train_set_train_speed(train_xmit_server_tid, request.train_num, request.train_speed);
        if (retval < 0) {
          bwprintf(COM2, "train controller - train speed failed: %d\n\r", retval);
        }

        retval = Reply(tid, (char *) &reply, 4);
        if (retval < 0) {
          bwprintf(COM2, "train controller - failed to reply: %d\n\r", retval);
        }

        retval = task_print_trains(print_server_tid, train_speed_map);
        if (retval < 0) {
          bwprintf(COM2, "train controller - failed to print trains: %d\n\r", retval);
        }
      }
    } else if (request.command == TRAIN_REVERSE_REQUEST_COMMAND) {
      // first unblock calling task
      reply = 0;
      retval = Reply(tid, (char *) &reply, 4);
      if (retval < 0) {
        bwprintf(COM2, "train controller - failed to reply: %d\n\r", retval);
      }

      retval = train_set_train_speed(train_xmit_server_tid, request.train_num, 0);
      if (retval < 0) {
        bwprintf(COM2, "train controller - rv train speed -> 0 failed: %d\n\r", retval);
      }

      rv_request.train_num = request.train_num;
      rv_request.restart_speed = train_speed_map[(int) request.train_num];
      rv_request.restart_time = Time(timer_server_tid) + 500; // 5 seconds
      retval = Send(reverse_controller_tid, (char *) &rv_request, sizeof(struct ReverseTrainRequest), (char *) &child_reply, 4);
      if (retval < 0) {
        bwprintf(COM2, "train controller - rv train send faied: %d\n\r", retval);
      }
    } else {
      // invalid command
      reply = -5;
      retval = Reply(tid, (char *) &reply, 4);
      if (retval < 0) {
        bwprintf(COM2, "train controller - failed to reply: %d\n\r", retval);
      }
    }
  }
  Exit();
}




void task__sensor_controller() {
  int print_server_tid = WhoIs(UART2_XMIT_SERVER_NS_NAME);
  if (print_server_tid < 0) {
    bwprintf(COM2, "Sensor - Failed to get print server tid: %d\n\r", print_server_tid);
  }

  int train_xmit_server_tid = WhoIs(UART1_XMIT_SERVER_NS_NAME);
  if (train_xmit_server_tid < 0) {
    bwprintf(COM2, "Sensor - Failed to get print server tid: %d\n\r", train_xmit_server_tid);
  }

  int train_receive_server_tid = WhoIs(UART1_RCV_SERVER_NS_NAME);
  if (train_receive_server_tid < 0) {
    bwprintf(COM2, "Sensor - Failed to get receive server tid: %d\n\r", train_receive_server_tid);
  }

  int timer_server_tid = WhoIs(TIMERSERVER_NS_NAME);
  if (timer_server_tid < 0) {
    bwprintf(COM2, "Timer Display - Failed to get timer server tid: %d\n\r", timer_server_tid);
  }

  int retval = 0;
  int expected_sensor_bytes = EXPECTED_DUMP_BYTES;
  int restart_dump_required = FALSE;

  char prev_sensor_data[EXPECTED_DUMP_BYTES]; memzero(prev_sensor_data, EXPECTED_DUMP_BYTES);
  char sensor_data[EXPECTED_DUMP_BYTES]; memzero(sensor_data, EXPECTED_DUMP_BYTES);

  char lsens[9];
  char input_char = '\0';
  struct ByteBuffer last_sensors;
  init_byte_buffer(&last_sensors, lsens, 9);

  for(;;) {
    if (restart_dump_required) {
      restart_dump_required = FALSE;
      expected_sensor_bytes = 10;

      retval = train_initiate_dump(train_xmit_server_tid, NUM_SENSOR_MODULES);
      if (retval < 0) {
        bwprintf(COM2, "Sensor - failed to initiate dump: %d\n\r", retval);
      }
    }

    input_char = retval = Getc(train_receive_server_tid, COM1);
    if (retval < 0) {
      bwprintf(COM2, "Sensor - Getc failed: %d\n\r", retval);
    }

    // swallow stray bytes if not dumping.
    if (expected_sensor_bytes > 0) {
      sensor_data[EXPECTED_DUMP_BYTES - expected_sensor_bytes] = input_char;
      expected_sensor_bytes--;
      if (expected_sensor_bytes == 0) {
        restart_dump_required = TRUE;

        int sensor_data_it;
        int sensor_byte_it;
        char sensor_dump_byte;
        char prev_sensor_dump_byte;
        int new_sensor_triggered = FALSE;
        for (sensor_data_it = 0; sensor_data_it < EXPECTED_DUMP_BYTES; sensor_data_it++) {
          sensor_dump_byte = sensor_data[sensor_data_it];
          prev_sensor_dump_byte = prev_sensor_data[sensor_data_it];
          for (sensor_byte_it = 0; sensor_byte_it < 8; sensor_byte_it++) {
            if (
              !(sensor_dump_byte & (1 << sensor_byte_it)) &&
              (prev_sensor_dump_byte & (1 << sensor_byte_it))
            ) {
              int sensor_block, sensor_num;
              char sensor_id;
              sensor_block = sensor_data_it / 2;
              sensor_num = ((sensor_data_it % 2) << 3) + (0x7 - sensor_byte_it);
              sensor_id = (char) ((sensor_block << 4) + sensor_num);
              byte_buffer_circular_push(&last_sensors, sensor_id);
              new_sensor_triggered = TRUE;
            }
          }
          prev_sensor_data[sensor_data_it] = sensor_dump_byte;
        }

        if (new_sensor_triggered) {
          retval = task_print_sensors(print_server_tid, &last_sensors);
          if (retval < 0) {
            bwprintf(COM2, "Sensor - failed to print sensors: %d\n\r", retval);
          }
          new_sensor_triggered = FALSE;
        }
      }
    }
  }

  Exit();
}

void task__track_init() {
  int retval = 0;

  int timer_server_tid = WhoIs(TIMERSERVER_NS_NAME);
  if (timer_server_tid < 0) {
    bwprintf(COM2, "Timer Display - Failed to get timer server tid: %d\n\r", timer_server_tid);
  }

  int print_server_tid = WhoIs(UART2_XMIT_SERVER_NS_NAME);
  if (print_server_tid < 0) {
    bwprintf(COM2, "Command Parser - Failed to get print server tid: %d\n\r", print_server_tid);
  }

  int train_xmit_server_id = WhoIs(UART1_XMIT_SERVER_NS_NAME);
  if (train_xmit_server_id < 0) {
    bwprintf(COM2, "switch controller - failed to whois: %d\n\r", train_xmit_server_id);
  }

  retval = task_init_layout_and_cursor(print_server_tid);
  if (retval < 0) {
    bwprintf(COM2, "Command Parser - init print failed: %d\n\r", retval);
  }

  int it;
  for (it = 1; it <= 18; it++) {
    train_set_switch_dir(train_xmit_server_id, it, 'C');
  }
  train_set_switch_dir(train_xmit_server_id, 153, 'C');
  train_set_switch_dir(train_xmit_server_id, 154, 'S');
  train_set_switch_dir(train_xmit_server_id, 155, 'C');
  train_set_switch_dir(train_xmit_server_id, 156, 'S');

  retval = train_solenoid_off(train_xmit_server_id);
  if (retval < 0) {
    bwprintf(COM2, "switch controller - failed to turn solenoid off: %d\n\r", retval);
  }

  retval = train_set_reset_mode(train_xmit_server_id, ON);
  if (retval < 0) {
    bwprintf(COM2, "Sensor - failed to set reset mode: %d\n\r", retval);
  }

  retval = train_initiate_dump(train_xmit_server_id, NUM_SENSOR_MODULES);
  if (retval < 0) {
    bwprintf(COM2, "switch controller - failed to initiate first dump: %d\n\r", retval);
  }

  volatile int *data = (int *)( UART1_BASE + UART_DATA_OFFSET );
  char input_char = *data;

  retval = Delay(timer_server_tid, 300);
  if (retval < 0) {
    bwprintf(COM2, "Sensor - delay failed: %d - %d.\n\r", retval, input_char);
  }

  // notify parent init finish
  retval = Send(MyParentTid(), NULL, 0, NULL, 0);
  if (retval < 0) {
    bwprintf(COM2, "track init - failed to notify parent: %d\n\r", retval);
  }

  Exit();
}

