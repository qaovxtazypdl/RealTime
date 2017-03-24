/* Non blocking send. */

#include <dispatch.h>
#include <common/syscall.h>
#include <common/mem.h>
#include <assert.h>

#define MAX_MSG_SZ 1024

struct seed {
  int td;
  char *msg;
  int msg_len;
};

static void dispatcher() {
  int td;
  struct seed seed;
  char msg[MAX_MSG_SZ];
  receive(&td, &seed, sizeof(seed));
  assert(seed.msg_len < MAX_MSG_SZ,
         "%d exceeds MAX_MSG_SZ", seed.msg_len);
  memcpy(msg, seed.msg, seed.msg_len);
  reply(td, NULL, 0);
  send(seed.td, msg, seed.msg_len, NULL, 0);
}

void dispatch(int td, void *buf, int buf_len) {
  int dtd;
  struct seed seed;

  dtd = create(0, dispatcher);
  seed.td = td;
  seed.msg_len = buf_len;
  seed.msg = buf;
  send(dtd, &seed, sizeof(seed), NULL, 0);
}
