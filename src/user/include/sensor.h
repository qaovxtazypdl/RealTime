#ifndef _SWITCH_H_
#define _SWITCH_H_

struct {
  int position;
  struct track_node *node;
} broken_swtich;

int switch_register_broken(struct broken_switch *switch);
int switch_get_broken(struct broken_switch switch[MAX_SWITCH]);

#endif
