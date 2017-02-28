/*
 * Mapping of registers on the versatile-pb to the ts7200. 
 * Since some of the peripherals are different (e.g the UARTs)
 * a direct mapping is not possible.
 *
 */

#define COM1	0
#define COM2	1

#define	TIMER1_BASE 0x101E2020
#define	TIMER2_BASE	0x101E2020
#define	TIMER3_BASE	0x101E2000 //Actually timer 0 (SP804)

#define	TIMER_INT_CLR	0xc

#define	LDR_OFFSET	0x00000000	/*  16/32 bits, RW */
#define	VAL_OFFSET	0x00000004	/*  16/32 bits, RO */
#define CRTL_OFFSET	0x00000008	/*  3 bits, RW */
  #define	ENABLE_MASK	0x00000080
  #define	MODE_MASK	0x000000E2
/*  make this idempotent for the emulator timers which don't allow for alteration of the clock speed */
  #define	CLKSEL_MASK	0x00000080 
#define CLR_OFFSET	0x0000000c	/*  no data, WO */

/* Map UART2 to UART0 on the emulated baseboard since this is the */
/* UART tied to the serial port. */
#define UART2_BASE 0x101F1000 
#define UART1_BASE 0x101F2000

#define UART_DATA_OFFSET	0x0	/*  low 8 bits */
  #define DATA_MASK	0xff
#define UART_RSR_OFFSET		0x4	/*  low 4 bits */
  #define FE_MASK		0x1
  #define PE_MASK		0x2
  #define BE_MASK		0x4
  #define OE_MASK		0x8
#define UART_LCRH_OFFSET	0x2c	/*  low 7 bits */
  #define BRK_MASK	0x1
  #define PEN_MASK	0x2	/*  parity enable */
  #define EPS_MASK	0x4	/*  even parity */
  #define STP2_MASK	0x8	/*  2 stop bits */
  #define FEN_MASK	0x10	/*  fifo */
  #define WLEN_MASK	0x60	/*  word length */
#define UART_FLAG_OFFSET	0x18	/*  low 8 bits */
  #define CTS_MASK	0x1
  #define DSR_MASK	0x2
  #define DCD_MASK	0x4
  #define TXBUSY_MASK	0x8
  #define RXFE_MASK	0x10	/*  Receive buffer empty */
  #define TXFF_MASK	0x20	/*  Transmit buffer full */
  #define RXFF_MASK	0x40	/*  Receive buffer full */
  #define TXFE_MASK	0x80	/*  Transmit buffer empty */

/*  Specific to UART1 */

#define UART_MDMCTL_OFFSET	0x100
#define UART_MDMSTS_OFFSET	0x104
#define UART_HDLCCTL_OFFSET	0x20c
#define UART_HDLCAMV_OFFSET	0x210
#define UART_HDLCAM_OFFSET	0x214
#define UART_HDLCRIB_OFFSET	0x218
#define UART_HDLCSTS_OFFSET	0x21c

#define VIC1_BASE  0x10140000
#define VIC2_BASE  0x10140000
#define VIC_ENABLE_OFFSET 0x10
#define VIC_SELECT_OFFSET 0xc
#define VIC_SOFTINT_OFFSET 0x18
#define VIC_ENCLEAR_OFFSET 0x14
#define VIC_IRQ_STATUS_OFFSET 0x0
#define VIC_SOFTINT_CLEAR_OFFSET 0x1C
#define VIC_TIMER3_EN 0x10
#define TIMER3_CLK_RATE 1000

#define VIC_UART1_EN 0x2000
#define VIC_UART2_EN 0x1000

/* Interrupt enable register */
#define UART_CTLR_OFFSET	0x38 /* Really IMSC offset in the PL011, but we can get away by treating it as CTRL as long as we use the appropriate bits */
#define PL011_UARTIMSC_OFFSET 0x38
    #define RIEN_MASK 0x10
    #define TIEN_MASK 0x20
    #define MSIEN_MASK 0x2 /* really the CTS interrupt in the pl011 */

#define PL011_UARTMIS_OFFSET 0x40
#define UART_INTR_OFFSET	0x40
    #define RX_INTR	0x10
    #define TX_INTR	0x20
    #define MDM_INTR 0x1 /* really the CTS interrupt in the pl011 */

#define PL011_IBRD 0x24
#define UART_LCRL_OFFSET 0x24
#define PL011_FBRD 0x28
#define UART_LCRM_OFFSET 0x28

void ts7200_enable_caches();
