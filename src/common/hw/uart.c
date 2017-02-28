#include <bwio.h>
#include <uart.h>
#include <ts7200.h>

#define COM2UARTBASE(com) ((com == COM2) ? UART2_BASE : UART1_BASE)

void inline ua_clear_cts(int channel) { UART_REG(channel, UART_INTR_OFFSET) = 1; }
void inline ua_cts_enable(int channel) { UART_REG(channel, UART_CTLR_OFFSET) |= MSIEN_MASK; }
void inline ua_cts_disable(int channel) { UART_REG(channel, UART_CTLR_OFFSET) &= (char)~MSIEN_MASK; }

void inline ua_txint_enable(int channel) { UART_REG(channel, UART_CTLR_OFFSET) |= TIEN_MASK; }
void inline ua_txint_disable(int channel) {UART_REG(channel, UART_CTLR_OFFSET) &= (char)~TIEN_MASK; }

void inline ua_rxint_disable(int channel) { UART_REG(channel, UART_CTLR_OFFSET) &= (char)~RIEN_MASK; }
/* void inline ua_rxint_enable(int channel) { UART_REG(channel, UART_CTLR_OFFSET) |= RIEN_MASK; } */

void inline ua_rxint_enable(int channel) {
  volatile int *int_reg = COM2UARTBASE(channel) + UART_CTLR_OFFSET;

  *int_reg |= RIEN_MASK;
}

/* TODO add logic to initialize other important uart params [e.g baud rate, parity] (currently relying on bwio) */
void ua_enable_interrupts(int channel) {
  ua_txint_enable(channel);
  ua_rxint_enable(channel);
  ua_cts_enable(channel);

  UART_REG(channel, UART_LCRH_OFFSET) &= (char)~FEN_MASK; /* Keep fifos disabled for now. */
}

/* void ua_set_bd(int channel, int baud_rate) { */
/*   UART_REG(COM1, UART_LCRM_OFFSET) = 0; */
/*   UART_REG(COM2, UART_LCRM_OFFSET) = 0; */
/*   UART_REG(COM2, UART_LCRL_OFFSET) = 0; */
/*   UART_REG(COM2, UART_LCRL_OFFSET) = 0; */
/* } */

void ua_init() {
  bwsetspeed( COM2, 115200 ); /* FIXME */
  bwsetfifo( COM2, 0 );

  bwsetspeed( COM1, 2400 );
  bwsetfifo( COM1, 0 );

  UART_REG(COM1, UART_LCRH_OFFSET) &= (char)~FEN_MASK;
  UART_REG(COM1, UART_LCRH_OFFSET) |= STP2_MASK;
}

int ua_read(int channel, char *c) {
  if(UART_REG(channel, UART_FLAG_OFFSET) & RXFE_MASK)
    return -1;

  *c = UART_REG(channel, UART_DATA_OFFSET);
  return 0;
}

int ua_write(int channel, char c) {
  if(UART_REG(channel, UART_FLAG_OFFSET) & TXFF_MASK)
    return -1;

  UART_REG(channel, UART_DATA_OFFSET) = c;
  return 0;
}
