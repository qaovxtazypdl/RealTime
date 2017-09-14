#ifndef _COMMON__UART_H_
#define _COMMON__UART_H_

//Obtain the baud rate divisor (BRD) [see cirrus doc] for the UART associated with the given COM channel
char ua_get_brd(int channel);

//Reverse engineer the clock rate based on the BRD and known baud rate of UART1. (hack, FIXME)
int ua_clk();

//Set/Clear control flags for the UART associated with the given COM channel.
void ua_setctrl(int channel, int set_mask, int clr_mask);

void ua_setint(int channel, int set_mask, int clr_mask);

//Set the baud rate for the given COM channel
int ua_set_bd(int channel, int rate);

int ua_write( int channel, char byte );

//Returns 0 on success
int ua_read(int channel, char *byte);

#endif
