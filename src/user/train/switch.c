#include <switch.h>
#include <common/syscall.h>

static int broken_switch_td = -1;

int switch_register_broken(struct broken_switch *b_switch) {
  if (broken_switch_td < 1 || b_switch == NULL || b_switch->node == NULL) return -5;
  return send(broken_switch_td, b_switch, sizeof(struct broken_switch), NULL, 0);
}

int switch_get_broken(struct broken_switch switches[MAX_SWITCH_SIZE]) {
  if (broken_switch_td < 1) return -5;
  struct broken_switch str;
  str.node = NULL;
  return send(broken_switch_td, &str, sizeof(struct broken_switch), switches, MAX_SWITCH_SIZE * sizeof(struct broken_switch));
}

void broken_switch_server() {
  struct broken_switch broken_switches[MAX_SWITCH_SIZE];
  int broken_switch_size = 0;
  broken_switches[0].node = NULL;

  struct broken_switch new_switch_data;
  int tid = -1;
  int i;
  while(1) {
    receive(&tid, &new_switch_data, sizeof(struct broken_switch));
    if (new_switch_data.node == NULL) {
      // GET
      reply(tid, broken_switches, (broken_switch_size + 1) * sizeof(struct broken_switch));
    } else {
      // REGISTER
      int switch_already_registered = 0;

      // check if already exists
      for (i = 0; i < broken_switch_size; i++) {
        if (new_switch_data.node == broken_switches[i].node) {
          switch_already_registered = 1;
          break;
        }
      }

      if (!switch_already_registered) {
        broken_switches[broken_switch_size].node = new_switch_data.node;
        broken_switches[broken_switch_size].position = new_switch_data.position;
        broken_switches[broken_switch_size + 1].node = NULL;
        broken_switch_size++;
      }

      reply(tid, NULL, 0);
    }
  }
}

void init_switches() {
  broken_switch_td = create(1, broken_switch_server);
}
