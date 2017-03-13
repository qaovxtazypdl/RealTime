#ifndef _ROUTING_H_
#define _ROUTING_H_
#include<track_node.h>

#define MAX_PATH_LEN 100

struct track_node *lookup_track_node(struct track_node *track, char *name);
int path_activate(struct track_node **path);
int path_find(struct track_node *track,
    struct track_node *src,
    struct track_node *dst,
    struct track_node **path);
#endif
