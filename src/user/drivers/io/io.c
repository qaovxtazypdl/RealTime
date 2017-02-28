#include <common/types.h>
#include <common/syscall.h>
#include <common/ds/queue.h>
#include <common/string.h>
#include <io.h>
#include "io.h"
#include <assert.h>
#include <ns.h>



static int os1, os2, is1, is2;
static int init = 0;

void init_io() {
  assert(init == 0, "init_io must have been called exactly once");

  int ch;

  os1 = create(0, output_server);
  os2 = create(0, output_server);
  is1 = create(0, input_server);
  is2 = create(0, input_server);

  
  ch = COM1;
  send(os1, &ch, sizeof(ch), NULL, 0);
  send(is1, &ch, sizeof(ch), NULL, 0);
  ch = COM2;
  send(os2, &ch, sizeof(ch), NULL, 0);
  send(is2, &ch, sizeof(ch), NULL, 0);

  init++;
}


/* Adds a contiugous list of characters to the buffer, useful for preventing interspersed characters 
   between tasks competing for output */

int putstr(int channel, char *str) {
  assert(init == 1, "init_io must have been called exactly once");

  int rc;
  struct osv_msg msg;
  int osv_td = (channel == COM1) ? os1 : os2;

  msg.op = OSV_PUTSTR;
  msg.data = str;

  rc = send(osv_td, (char*)&msg, sizeof(msg), NULL, 0);
  assert(rc == 0, "non-null message returned by %d", osv_td);
  return rc;
}

int putc(int channel, char c) {
  assert(init == 1, "init_io must have been called exactly once");

  int rc;
  struct osv_msg msg;
  int osv_td = (channel == COM1) ? os1 : os2;

  msg.op = OSV_PUTC;
  msg.data = &c;

  rc = send(osv_td, &msg, sizeof(msg), NULL, 0);
  assert(rc == 0, "Got %d bytes in %d from %d", rc, my_tid(), osv_td);
  return rc;
}

int getc(int channel, char *c) {
  assert(init == 1, "init_io must have been called exactly once");

  isv_msg_t msg;
  int isv_td = (channel == COM1) ? is1 : is2;

  msg.op = ISV_OP_GETC;
  if(send(isv_td, &msg, sizeof(msg), c, (c == NULL) ? 0 : 1) == 1)
    return 0;

  return 1;
}
