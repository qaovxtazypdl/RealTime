#include <io.h>
#include "io.h"
#include <ns.h>
#include <assert.h>
#include "../notifier.h"
#include <common/syscall.h>
#include <common/ds/queue.h>

#define INPUT_SUBSCRIBER_SZ 10
#define INPUT_BUFFER_SZ 10

void input_server() {
  int tx_td;
  char c;
  isv_msg_t msg;
  int td, channel, sub, rx_td;
  queue_t input_subscribers, input_buffer;
  int input_subscribers_mem[INPUT_SUBSCRIBER_SZ];
  char input_buffer_mem[INPUT_BUFFER_SZ];

  queue_init(&input_buffer, 
             input_buffer_mem,
             sizeof(input_buffer_mem),
             sizeof(input_buffer_mem[0]));

  queue_init(&input_subscribers, 
             input_subscribers_mem, 
             sizeof(input_subscribers_mem), 
             sizeof(input_subscribers_mem[0]));


  receive(&td, &channel, sizeof(channel));
  reply(td, NULL, 0);
  assert(channel == COM2 || channel == COM1, "Invalid channel");

  if(channel == COM2) {
    rx_td = create_notifier(0, EVENT_UART2_RX, &c);
    register_as("input_server_2");
  } else {
    rx_td = create_notifier(0, EVENT_UART1_RX, &c);
    register_as("input_server_1");
  }

  while(1) {
    receive(&td, &msg, sizeof(msg));

    if(td == rx_td) {
      if(!queue_consume(&input_subscribers, &sub))
        reply(sub, &c, 1);
      else
        queue_add(&input_buffer, &c);

      reply(td, NULL, 0);
    } else {
      if(!queue_consume(&input_buffer, &c))
        reply(td, &c, 1);
      else
        queue_add(&input_subscribers, &td);
    }
  }
}
