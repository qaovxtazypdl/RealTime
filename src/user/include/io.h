#ifndef _IO_H_
#define _IO_H_

#define IO_OUTPUT_BUFFER_SZ 2048
#define COM2UARTBASE(com) ((com == COM1) ? UART1_BASE : UART2_BASE)

#define COM1 0
#define COM2 1

enum osv_op {
  OSV_PUTC,
  OSV_PUTSTR
};

struct osv_msg {
  enum osv_op op;
  /* Interpreted as a string in the case of PUTSTR and a pointer to a character in the case of PUTC */
  char *data; 
};

enum isv_op {
  ISV_OP_GETC,
  ISV_OP_RX_NOTIFICATION
};

typedef struct isv_msg_t {
  enum  isv_op op;
  char *data;
} isv_msg_t;

int getc(int channel, char *c);
int putc(int channel, char c);
int putstr(int channel, char *c);
int putstr(int channel, char *c);
void printf(int channel, char *fmt, ...);
int sprintf(char *buf, char *fmt, ...);

int create_input_server(int channel);
void simple_output_server();
void simple_cts_output_server();
void init_io();

#define ERR_INVALID_CHANNEL -1

#define MOVE_TO(x,y) "\x1B["#y";"#x"H"
#define SAVE_CURSOR "\x1B[s"
#define RESTORE_CURSOR "\x1B[u"
#define CLEAR_SCREEN "\x1B[2J"
#define CLEAR_LINE "\x1B[2K\r"
#define CLEAR_DOWN "\x1B[J"
#define CLEAR_EOL "\x1B[K"
#endif
