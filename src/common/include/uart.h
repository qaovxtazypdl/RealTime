#ifndef _UART_H_
#define _UART_H_

#define COM1	0
#define COM2	1
#define UART_REG(com, offset) *((volatile char*)(((com == COM2) ? UART2_BASE : UART1_BASE) + offset))

int ua_read(int channel, char *c);
int ua_write(int channel, char c);
void ua_clear_cts(int channel);
void ua_cts_enable(int channel);
void ua_cts_disable(int channel);
void ua_txint_enable(int channel);
void ua_txint_disable(int channel);
void ua_rxint_disable(int channel);
void ua_rxint_enable(int channel);
void ua_enable_interrupts(int channel);
void ua_init();

#endif
