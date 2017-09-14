#ifndef _TASKS__UART_SERVER_NOTIFIER_H_
#define _TASKS__UART_SERVER_NOTIFIER_H_

#define UART_NOTIFY ((char) 0)
#define UART_RECEIVE ((char) 1)
#define UART_TRANSMIT ((char) 2)

// 0-3 reserved for commands
#define UART_1_COMMAND_MASK 0x4
#define UART_2_COMMAND_MASK 0x8

void task__uart_initializer();

// packet structure is variable length. first byte of string defines the type.
void task__uart_receive_server();
void task__uart_transmit_server();

void task__uart1_ready_notifier();
void task__uart1_receive_notifier();
void task__uart1_transmit_notifier();
void task__uart1_modem_notifier();

void task__uart2_receive_notifier();
void task__uart2_transmit_notifier();


int Getc(int tid, int uart);

int Putc(int tid, int uart, char ch);
// do not call directly - use printf.h
int PutStr(int tid, int uart, char *str, int len);

#endif
