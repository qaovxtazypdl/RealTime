#include <assert.h>
#include "notifier.h"
#include <common/event.h>
#include <common/syscall.h>

struct notifier_seed {
  enum event ev;
  void *payload;
};


static void notifier() {
  int ptd;
  int rc;
  struct notifier_seed seed;
  receive(&ptd, &seed, sizeof(seed));
  reply(ptd, NULL, 0);

  while(1) {
    await_event(seed.ev, seed.payload);
    rc = send(ptd, NULL, 0, NULL, 0);
    assert(!rc, "send failed: %d", rc);
  }
}

/* Usage: Creates a task which sends an empty message to the parent
   each time the provided event occurs. The payload in the seed is the
   payload that gets passed to the await event call, and should be a
   pointer to a piece of memory in the parent task where the data
   associated with the interrupt will be stored after each
   notification. */

int create_notifier(int priority, enum event ev, void *payload) {
  int td = create(priority, notifier);
  send(td, &(struct notifier_seed) { ev, payload },
       sizeof(struct notifier_seed),
       NULL, 0);
  return td;
}
