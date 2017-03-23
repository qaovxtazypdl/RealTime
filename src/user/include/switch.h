#ifndef _SWITCH_H_
#define _SWITCH_H_

#define NUM_SWITCHES 22
#define MAX_SWITCH_SIZE (NUM_SWITCHES + 1)

struct broken_switch {
  int position;
  struct track_node *node;
};

int switch_register_broken(struct broken_switch *b_switch);
int switch_get_broken(struct broken_switch switches[MAX_SWITCH_SIZE]);
void init_switches();

#endif
