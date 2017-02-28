#include <common/syscall.h>
#include <common/ds/queue.h>
#include <common/string.h>
#include <common/uart.h>
#include <ns.h>
#include <io.h>
#include "../notifier.h"
#include <assert.h>

#define OBUF_SZ 4096

static void write_char(int *tx, int *cts, int tx_td,
                       int cts_td, queue_t *q, int channel) {
  char c;

  
  if(*tx && *cts && !queue_consume(q, &c)) {
    ua_write(channel, c);

    if(channel == COM1) {
      reply(cts_td, NULL, 0);
      *cts = 0;
    }
    *tx = 0;
    reply(tx_td, NULL, 0);
  }
}

void output_server() {
  queue_t buf;
  struct osv_msg msg;
  char buf_mem[OBUF_SZ];
  int td, full, channel, tx_td, cts_td;

  int cts = 0;
  int tx = 0;

  queue_init(&buf, buf_mem, OBUF_SZ, 1);
  receive(&td, &channel, sizeof(channel));
  reply(td, NULL, 0);
  if(channel == COM2) cts = 1;


  assert(channel == COM1 || channel == COM2, "Invalid channel");

  if(channel == COM1) {
    register_as("output_server_1");
    tx_td = create_notifier(0, EVENT_UART1_TX, NULL);
    cts_td = create_notifier(0, EVENT_UART1_CTS, NULL);
  } else {
    register_as("output_server_2");
    tx_td = create_notifier(0, EVENT_UART2_TX, NULL);
  } 

  while(1) {
    receive(&td, &msg, sizeof(msg));

    if(td == tx_td) {
      tx = 1;
      write_char(&tx, &cts, tx_td, cts_td, &buf, channel);
    } else if((channel == COM1) && (td == cts_td)) {
      cts = 1;
      write_char(&tx, &cts, tx_td, cts_td, &buf, channel);
    } else {
      if(msg.op == OSV_PUTSTR)
        full = queue_add_multiple(&buf, msg.data, strlen(msg.data));
      else if(msg.op == OSV_PUTC)
        full = queue_add(&buf, msg.data);
      else
        assert(0, "Something very bad has happened :/");

      assert(!full, "Output buffer for COM%d is full!", channel + 1);

      reply(td, NULL, 0);
      write_char(&tx, &cts, tx_td, cts_td, &buf, channel);
    }
  }
}

