#ifndef _TRAIN_H_
#define _TRAIN_H_

#define TRAIN_COM COM1
#include <track_node.h>

struct position {
  struct track_node *node;
  int offset;
};

struct train_command {
  enum train_command_type {
    TRAIN_COMMAND_SET_PATH
  } type;
  struct track_node **path;
};
#endif
