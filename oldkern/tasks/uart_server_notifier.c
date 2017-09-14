#include "tasks/uart_server_notifier.h"
#include "tasks/name_server.h"
#include "tasks/utils/ns_names.h"
#include "common/constants.h"
#include "common/assert.h"
#include "emul/ts7200.h"
#include "common/algs.h"
#include "bwio/bwio.h"
#include "kernel/syscall.h"
#include "common/byte_buffer.h"
#include "tasks/utils/printf.h"
#include "tasks/timer_server_notifier.h"


#define UART1SM_CTS_NEGATED 0
#define UART1SM_CTS_ASSERTED 1
#define UART1SM_TXFE_ASSERTED 2



void task__uart_initializer() {
  volatile int *uart1_int_reg = (int *)(UART1_BASE + UART_CTLR_OFFSET);
  volatile int *uart2_int_reg = (int *)(UART2_BASE + UART_CTLR_OFFSET);
  volatile int *uart1_status = (int *)( UART1_BASE + UART_RSR_OFFSET );
  volatile int *uart2_status = (int *)( UART2_BASE + UART_RSR_OFFSET );

  *uart1_status = 0;
  *uart2_status = 0;

  uart1_init_fifo(OFF);
  uart2_init_fifo(OFF);
  uart1_set_speed();
  uart2_set_speed();

  *uart2_int_reg |= TIEN_MASK | RIEN_MASK;
  *uart1_int_reg |= TIEN_MASK | RIEN_MASK | MSIEN_MASK;

  Exit();
}

int NotifyUART1StateMachineCTSNegation(int tid) {
  char request = UART1SM_CTS_NEGATED;
  int reply;
  int retval = 0;
  retval = Send(tid, &request, 1, (char *) &reply, 4);

  return retval < 0 ? retval : reply;
}

int NotifyUART1StateMachineCTSReassert(int tid) {
  char request = UART1SM_CTS_ASSERTED;
  int reply;
  int retval = 0;
  retval = Send(tid, &request, 1, (char *) &reply, 4);

  return retval < 0 ? retval : reply;
}

int NotifyUART1StateMachineXMITReady(int tid) {
  char request = UART1SM_TXFE_ASSERTED;
  int reply;
  int retval = 0;
  retval = Send(tid, &request, 1, (char *) &reply, 4);

  return retval < 0 ? retval : reply;
}

int Getc(int tid, int uart) {
  ASSERT(uart == COM1 || uart == COM2, "invalid uart: %d", uart);
  int result = -1;
  int reply = -1;

  char request = UART_RECEIVE | (uart == COM1? UART_1_COMMAND_MASK : UART_2_COMMAND_MASK);

  result = Send(tid, &request, 1, (char *) &reply, 4);

  if (result == -2) {
    return -1;
  } else if (result < 0 || reply < 0) {
    return result >= 0 ? reply : result;
  } else {
    return reply;
  }
}

int Putc(int tid, int uart, char ch) {
  char request[2];
  request[0] = UART_TRANSMIT | (uart == COM1? UART_1_COMMAND_MASK : UART_2_COMMAND_MASK);
  request[1] = ch;

  return PutStr(tid, uart, request, 2);
}

// do not call directly - use printf.h
int PutStr(int tid, int uart, char *str, int len) {
  ASSERT(uart == COM1 || uart == COM2, "invalid uart: %d", uart);
  int result = -1;
  int reply = -1;

  result = Send(tid, str, len, (char *) &reply, 4);

  if (result == -2) {
    return -1;
  } else if (result < 0 || reply < 0) {
    return result >= 0 ? reply : result;
  } else {
    return 0;
  }
}

int NotifyReceiveServer(int tid, int uart, char input) {
  ASSERT(uart == COM1 || uart == COM2, "invalid uart: %d", uart);
  int result = -1;
  int reply = -1;
  char request[2];
  request[0] = UART_NOTIFY | (uart == COM1? UART_1_COMMAND_MASK : UART_2_COMMAND_MASK);
  request[1] = input;

  result = Send(tid, request, 2, (char *) &reply, 4);
  if (result == -2) {
    return -1;
  } else if (result < 0 || reply < 0) {
    return result >= 0 ? reply : result;
  } else {
    return 0;
  }
}


int NotifyTransmitServer(int tid, int uart) {
  ASSERT(uart == COM1 || uart == COM2, "invalid uart: %d", uart);
  int result = -1;
  int reply = -1;
  char request = UART_NOTIFY | (uart == COM1? UART_1_COMMAND_MASK : UART_2_COMMAND_MASK);

  result = Send(tid, &request, 1, (char *) &reply, 4);

  if (result == -2) {
    return -1;
  } else if (result < 0 || reply < 0) {
    return result >= 0 ? reply : result;
  } else {
    return 0;
  }
}

void task__uart_receive_server() {
  int retval;

  int uart = 0;
  retval = Send(MyParentTid(), NULL, 0, (char *) &uart, 1);
  if (retval != 1) {
    bwprintf(COM2, "UART-%d RCV Server - Failed to initialize. %d!\n\r", uart, retval);
  }

  retval = RegisterAs(uart == COM1 ? UART1_RCV_SERVER_NS_NAME : UART2_RCV_SERVER_NS_NAME);
  if (retval < 0) {
    bwprintf(COM2, "UART-%d RCV Server - RegisterAs failed: %d!\n\r", uart, retval);
  }

  int xmit_server_tid;
  xmit_server_tid = WhoIs(uart == COM1 ? UART1_XMIT_SERVER_NS_NAME : UART2_XMIT_SERVER_NS_NAME );
  if (xmit_server_tid < 0) {
    bwprintf(COM2, "UART-%d RCV Server - XMIT Server ID not found: %d.\n\r", uart, xmit_server_tid);
  }

  char getc_request_tid_buffer[4096];
  struct ByteBuffer getc_request_tids;
  init_byte_buffer(&getc_request_tids, getc_request_tid_buffer, 4096);

  char receive_internal_buffer[4096];
  struct ByteBuffer receive_buffer;
  init_byte_buffer(&receive_buffer, receive_internal_buffer, 4096);

  int tid = 0;
  char request[2];
  int reply = -1;
  char uart_input;

  for (;;) {
    retval = Receive(&tid, request, 2);
    if (retval < 0) {
      bwprintf(COM2, "UART-%d RCV Server - Receive failed: %d!\n\r", uart, retval);
    }

    ASSERT(uart == COM1 ? request[0] & UART_1_COMMAND_MASK : request[0] & UART_2_COMMAND_MASK, "wrong uart specified for UART-%d RCV Server: %x, tid: %d", uart, request[0], tid);
    switch (request[0] % 4) {
      case UART_NOTIFY:
        // receive ready - put into buffer
        uart_input = request[1];
        if (is_byte_buffer_empty(&receive_buffer) && !is_byte_buffer_empty(&getc_request_tids)) {
          // getc blocked on input - unblock and send
          // reply to notifier
          reply = 0;
          retval = Reply(tid, (char *) &reply, 4);
          if (retval < 0) {
            bwprintf(COM2, "UART-%d RCV Server - Reply failed: %d!\n\r", uart, retval);
          }
          // reply to getc
          reply = (int) uart_input;
          retval = Reply(byte_buffer_pop(&getc_request_tids), (char *) &reply, 4);
        } else if (is_byte_buffer_empty(&getc_request_tids)) {
          // input buffered waiting for getc
          if (is_byte_buffer_full(&receive_buffer)) {
            reply = -3;
          } else {
            reply = 0;
            byte_buffer_push(&receive_buffer, uart_input);
          }
          // reply to notifier
          retval = Reply(tid, (char *) &reply, 4);
        } else {
          ASSERT(0, "UART-%d RCV Server - Both buffer and getc queue are nonempty - INVARIANTVIOLATION", uart);
          // reply to notifier
          reply = -5;
          retval = Reply(tid, (char *) &reply, 4);
        }
        break;
      case UART_RECEIVE:
        if (!is_byte_buffer_empty(&receive_buffer)) {
          // buffer full - pop ONE from buffer and return
          // reply to getc
          reply = (int) byte_buffer_pop(&receive_buffer);
          retval = Reply(tid, (char *) &reply, 4);
        } else {
          // buffer empty - queue it up and block task
          if (is_byte_buffer_full(&getc_request_tids)) {
            // dont block - error out
            reply = -3;
            retval = Reply(tid, (char *) &reply, 4);
          } else {
            byte_buffer_push(&getc_request_tids, tid);
          }
        }
        break;
      default:
        // reply error to the request
        reply = -4;
        retval = Reply(tid, (char *) &reply, 4);
        break;
    }
    if (retval < 0) {
      bwprintf(COM2, "UART-%d RCV Server - Reply failed: %d, reply: %d!\n\r", uart, retval, reply);
    }
  }

  Exit();
}

void task__uart_transmit_server() {
  int retval;

  int uart = 0;
  retval = Send(MyParentTid(), NULL, 0, (char *) &uart, 1);
  if (retval != 1) {
    bwprintf(COM2, "UART-%d XMIT Server - Failed to initialize. %d!\n\r", uart, retval);
  }

  retval = RegisterAs(uart == COM1 ? UART1_XMIT_SERVER_NS_NAME : UART2_XMIT_SERVER_NS_NAME);
  if (retval < 0) {
    bwprintf(COM2, "UART-%d XMIT Server - RegisterAs failed: %d!\n\r", uart, retval);
  }

  int timer_server_tid = WhoIs(TIMERSERVER_NS_NAME);
  if (timer_server_tid < 0) {
    bwprintf(COM2, "UART-%d XMIT Server - Failed to get timer server tid: %d\n\r", uart, timer_server_tid);
  }

  char internal_transmit_buffer[4096];
  struct ByteBuffer transmit_buffer;
  init_byte_buffer(&transmit_buffer, internal_transmit_buffer, 4096);

  int tid = 0;
  char request[256]; //TODO: fixme
  int reply = -1;
  int xmit_ready = TRUE; //assume interrupt has already been raised prior
  volatile int *uart_int_reg = (int *)((uart == COM1 ? UART1_BASE : UART2_BASE) + UART_CTLR_OFFSET);
  volatile int *write_reg = (int *)((uart == COM1 ? UART1_BASE : UART2_BASE) + UART_DATA_OFFSET);
  volatile int *flags = (int *)((uart == COM1 ? UART1_BASE : UART2_BASE) + UART_FLAG_OFFSET );
  char char_to_write;
  int message_length = 0;
  int iterator;

  int last_notify = 0;

  for (;;) {
    // retval holds length of transmission
    message_length = retval = Receive(&tid, request, 256);
    if (retval < 0) {
      bwprintf(COM2, "UART-%d XMIT Server - Receive failed from tid: %d - %d!\n\r", uart, tid, retval);
    }

    ASSERT(uart == COM1 ? request[0] & UART_1_COMMAND_MASK : request[0] & UART_2_COMMAND_MASK, "wrong uart specified for UART-%d XMIT Server: %x, tid: %d", uart, request[0], tid);
    switch(request[0] % 4) {
      case UART_NOTIFY:
        reply = 0;
        retval = Reply(tid, (char *) &reply, 4);

        last_notify = Time(timer_server_tid);
        if (last_notify < 0) {
          bwprintf(COM2, "UART-%d XMIT Server - time failed: %d.\n\r", uart, last_notify);
        }

        if (!is_byte_buffer_empty(&transmit_buffer)) {
          ASSERT(*flags & TXFE_MASK, "UART-%d Transmit buffer full, but ready - notify", uart);

          // pop from buffer and write
          char_to_write = byte_buffer_pop(&transmit_buffer);
          *write_reg = char_to_write;
          xmit_ready = FALSE;
          // re-enable interrupt after write
          *uart_int_reg |= TIEN_MASK;
        } else {
          xmit_ready = TRUE;
        }
        break;
      case UART_TRANSMIT:
        if (transmit_buffer.len - byte_buffer_count(&transmit_buffer) < message_length) {
          reply = -3;
          retval = Reply(tid, (char *) &reply, 4);
          break;
        }

        // reply before heavy computation in copying
        reply = 0;
        retval = Reply(tid, (char *) &reply, 4);

        for (iterator = 1; iterator < message_length; iterator++) {
          if (!is_byte_buffer_full(&transmit_buffer)) {
            byte_buffer_push(&transmit_buffer, request[iterator]);
          } else {
            ASSERT(0, "UART-%d XMIT Server - Buffer should not have been full.", uart);
            break;
          }
        }

        int cur_time = Time(timer_server_tid);
        if (cur_time < 0) {
          bwprintf(COM2, "UART-%d XMIT Server - time failed: %d.\n\r", uart, cur_time);
        }

        if (cur_time - last_notify > (uart == COM1 ? 20 : 2)) {
          xmit_ready = TRUE;
        }

        if (xmit_ready) {
          // pop from buffer and write
          ASSERT(*flags & TXFE_MASK, "UART-%d Transmit buffer full, but ready - transmit", uart);

          char_to_write = byte_buffer_pop(&transmit_buffer);
          *write_reg = char_to_write;
          xmit_ready = FALSE;
          // re-enable interrupt after write
          *uart_int_reg |= TIEN_MASK;
        }
        break;
      default:
        // reply error to the request
        reply = -4;
        retval = Reply(tid, (char *) &reply, 4);
        break;
    }
    if (retval < 0) {
      bwprintf(COM2, "UART-%d XMIT Server - Reply failed: %d, reply: %d, repl to: %d!\n\r", retval, reply, tid, uart);
    }
  }
  Exit();
}

void task__uart2_receive_notifier() {
  int server_tid;
  server_tid = WhoIs(UART2_RCV_SERVER_NS_NAME);
  if (server_tid < 0) {
    bwprintf(COM2, "UART 2 RCV Notifier - Server ID not found: %d.\n\r", server_tid);
  }

  int retval = 0;
  for (;;) {
    // retval has value of the character read
    retval = AwaitEvent(EVENT_UART2_RCV_RDY);
    if (retval < 0) {
      bwprintf(COM2, "UART 2 RCV Notifier - AwaitEvent failed: %d!\n\r", retval);
    }
    retval = NotifyReceiveServer(server_tid, COM2, retval);
    if (retval < 0) {
      bwprintf(COM2, "UART 2 RCV Notifier - NotifyReceiveServer failed: %d!\n\r", retval);
    }
  }
}

void task__uart2_transmit_notifier() {
  int server_tid;
  server_tid = WhoIs(UART2_XMIT_SERVER_NS_NAME);
  if (server_tid < 0) {
    bwprintf(COM2, "UART 2 XMIT Notifier - Server ID not found: %d.\n\r", server_tid);
  }

  int retval = 0;
  for (;;) {
    retval = AwaitEvent(EVENT_UART2_XMIT_RDY);
    if (retval < 0) {
      bwprintf(COM2, "UART 2 XMIT Notifier - AwaitEvent failed: %d!\n\r", retval);
    }

    retval = NotifyTransmitServer(server_tid, COM2);
    if (retval < 0) {
      bwprintf(COM2, "UART 2 XMIT Notifier - NotifyTransmitServer failed: %d!\n\r", retval);
    }
  }
}


// modem and transmit send to ready, ready sends to server when cts flips
void task__uart1_ready_notifier() {
  int retval;
  int tid;

  retval = RegisterAs(UART1_XMIT_READY_NOTIFIER_NS_NAME);
  if (retval < 0) {
    bwprintf(COM2, "UART1 Ready Server - RegisterAs failed: %d!\n\r", retval);
  }

  int server_tid;
  server_tid = WhoIs(UART1_XMIT_SERVER_NS_NAME);
  if (server_tid < 0) {
    bwprintf(COM2, "UART1 Ready Notifier - Server ID not found: %d.\n\r", server_tid);
  }

  char request;
  int reply = 0;

  int has_cts_negated = FALSE;
  int has_cts_reasserted = FALSE;
  int has_txfe_reasserted = FALSE;

  for (;;) {
    retval = Receive(&tid, &request, 1);
    if (retval < 0) {
      bwprintf(COM2, "UART1 Ready Notifier - Receive failed!\n\r", retval);
    }

    switch (request) {
      case UART1SM_CTS_NEGATED:
        has_cts_negated = TRUE;
        reply = 0;
        break;
      case UART1SM_CTS_ASSERTED:
        ASSERT(has_cts_negated, "CTS asserted before negation!");
        has_cts_reasserted = TRUE;
        reply = 0;
        break;
      case UART1SM_TXFE_ASSERTED:
        has_txfe_reasserted = TRUE;
        reply = 0;
        break;
      default:
        reply = -5;
        break;
    }

    retval = Reply(tid, (char *) &reply, 4);
    if (retval < 0) {
      bwprintf(COM2, "UART1 Ready Notifier - Reply failed!\n\r", retval);
    }

    if (has_cts_negated && has_cts_reasserted && has_txfe_reasserted) {
      retval = NotifyTransmitServer(server_tid, COM1);
      if (retval < 0) {
        bwprintf(COM2, "UART1 Ready Notifier - NotifyTransmitServer failed: %d!\n\r", retval);
      }

      has_cts_negated = FALSE;
      has_cts_reasserted = FALSE;
      has_txfe_reasserted = FALSE;
    }
  }
}

void task__uart1_modem_notifier() {
  int notifier_tid;
  int previous_cts = TRUE;
  int current_cts;
  volatile int *flags = (int *)( UART1_BASE + UART_FLAG_OFFSET );

  notifier_tid = WhoIs(UART1_XMIT_READY_NOTIFIER_NS_NAME);
  if (notifier_tid < 0) {
    bwprintf(COM2, "UART 1 XMIT Notifier - Server ID not found: %d.\n\r", notifier_tid);
  }

  int retval = 0;
  for (;;) {
    retval = AwaitEvent(EVENT_UART1_MODEM);
    if (retval < 0) {
      bwprintf(COM2, "UART 1 XMIT Notifier - AwaitEvent failed: %d!\n\r", retval);
    }

    // check current state
    current_cts = *flags & CTS_MASK;

    if (current_cts && !previous_cts) {
      retval = NotifyUART1StateMachineCTSReassert(notifier_tid);
      if (retval < 0) {
        bwprintf(COM2, "UART 1 XMIT Notifier - NotifyUART1StateMachineCTSReassert failed: %d!\n\r", retval);
      }
    } else if (!current_cts && previous_cts) {
      retval = NotifyUART1StateMachineCTSNegation(notifier_tid);
      if (retval < 0) {
        bwprintf(COM2, "UART 1 XMIT Notifier - NotifyUART1StateMachineCTSNegation failed: %d!\n\r", retval);
      }
    } else {
      // CTS didnt change - ignore
    }

    previous_cts = current_cts;
  }
}

void task__uart1_transmit_notifier() {
  int notifier_tid;
  notifier_tid = WhoIs(UART1_XMIT_READY_NOTIFIER_NS_NAME);
  if (notifier_tid < 0) {
    bwprintf(COM2, "UART 1 XMIT Notifier - Server ID not found: %d.\n\r", notifier_tid);
  }

  int retval = 0;
  for (;;) {
    retval = AwaitEvent(EVENT_UART1_XMIT_RDY);
    if (retval < 0) {
      bwprintf(COM2, "UART 1 XMIT Notifier - AwaitEvent failed: %d!\n\r", retval);
    }

    retval = NotifyUART1StateMachineXMITReady(notifier_tid);
    if (retval < 0) {
      bwprintf(COM2, "UART 1 XMIT Notifier - NotifyUART1StateMachineXMITReady failed: %d!\n\r", retval);
    }
  }
}


void task__uart1_receive_notifier() {
  int server_tid;
  server_tid = WhoIs(UART1_RCV_SERVER_NS_NAME);
  if (server_tid < 0) {
    bwprintf(COM2, "UART 1 RCV Notifier - Server ID not found: %d.\n\r", server_tid);
  }

  int retval = 0;
  for (;;) {
    // retval has value of the character read
    retval = AwaitEvent(EVENT_UART1_RCV_RDY);
    if (retval < 0) {
      bwprintf(COM2, "UART 1 RCV Notifier - AwaitEvent failed: %d!\n\r", retval);
    }
    retval = NotifyReceiveServer(server_tid, COM1, retval);
    if (retval < 0) {
      bwprintf(COM2, "UART 1 RCV Notifier - NotifyReceiveServer failed: %d!\n\r", retval);
    }
  }
}


