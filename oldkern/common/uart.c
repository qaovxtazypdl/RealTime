#include "common/uart.h"
#include "emul/ts7200.h"
#include "bwio/bwio.h"

//Obtain the baud rate divisor (BRD) [see cirrus doc] for the UART associated with the given COM channel
char ua_get_brd(int channel) {
	char *base;

	if(channel == COM1)
		base = (char *)UART1_BASE;
	else
		base = (char *)UART2_BASE;

	char *b1 = (char *) (base + UART_LCRM_OFFSET);
	char *b2 = (char *) (base + UART_LCRL_OFFSET);

	return (((char)*b1) << 8) | *b2;
}

//Reverse engineer the clock rate based on the BRD and known baud rate of UART1. (hack, FIXME)
int ua_clk() {
	char brd = ua_get_brd(COM2);
	return (brd + 1) * 16 * 115200; //Known baud rate of UART1
}

//Set/Clear control flags for the UART associated with the given COM channel.
void ua_setctrl(int channel, int set_mask, int clr_mask) {
	char *base = (char *)((channel == COM1) ? UART1_BASE : UART2_BASE);
	int *reg = (int*)(base + UART_LCRH_OFFSET);
	*reg |= set_mask;
	*reg &= ~clr_mask;
}

/*
  #define MSIEN_MASK  0x8 // modem status int
  #define RIEN_MASK 0x10  // receive int
  #define TIEN_MASK 0x20  // transmit int
  #define RTIEN_MASK  0x40  // receive timeout int
*/
void ua_setint(int channel, int set_mask, int clr_mask) {
  char *base = (char *)((channel == COM1) ? UART1_BASE : UART2_BASE);
  int *reg = (int*)(base + UART_CTLR_OFFSET);
  *reg |= set_mask;
  *reg &= ~clr_mask;
}

//Set the baud rate for the given COM channel
int ua_set_bd(int channel, int rate) {
	int uart_clk = ua_clk();
	char brd = (uart_clk / (rate << 4)) - 1; //Calculate baud rate divisor based on the given baud rate
	char *base;

	if(channel == COM1)
		base = (char *)UART1_BASE;
	else
		base = (char *)UART2_BASE;

	char *b1 = (char *) (base + UART_LCRM_OFFSET);
	char *b2 = (char *) (base + UART_LCRL_OFFSET);

	*b1 = (brd >> 8);
	*b2 = brd;

	int *high_reg = (int*) (base + UART_LCRH_OFFSET); //Needs to be written for L/M register changes to take effect
	*high_reg = *high_reg;

	return 0;
}

int ua_write( int channel, char byte ) {
	int *flags, *data;
	switch( channel ) {
	case COM1:
		flags = (int *)( UART1_BASE + UART_FLAG_OFFSET );
		data = (int *)( UART1_BASE + UART_DATA_OFFSET );
		break;
	case COM2:
		flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
		data = (int *)( UART2_BASE + UART_DATA_OFFSET );
		break;
	default:
		return 1;
		break;
	}

	if( *flags & TXFF_MASK )
		return 1;

	*data = byte;
	return 0;
}

//Returns 0 on success
int ua_read(int channel, char *byte) {
	int *flags, *data;

	switch( channel ) {
	case COM1:
		flags = (int *)( UART1_BASE + UART_FLAG_OFFSET );
		data = (int *)( UART1_BASE + UART_DATA_OFFSET );
		break;
	case COM2:
		flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
		data = (int *)( UART2_BASE + UART_DATA_OFFSET );
		break;
	default:
		return -1;
		break;
	}
	if ( !( *flags & RXFF_MASK ) ) //Nothing to read
		return -2;

	*byte = *data;
	return 0;
}
