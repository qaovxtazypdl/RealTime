#ifndef _ROUTING_H_
#define _ROUTING_H_
#include <track_node.h>

#define MAX_PATH_LEN 100

struct track_node *lookup_track_node(char *name);
int path_activate(struct track_node **path);
/* Returns the number of nodes contained in the returned path, this is zero if no path exists. */
int path_find(struct track_node *src,
              struct track_node *dst,
              struct track_node **path);
#endif
