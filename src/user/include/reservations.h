#ifndef _RESERVATIONS_H_
#define _RESERVATIONS_H_

#include <track_node.h>

int reserve_track(struct track_node *track);
int free_track(struct track_node *track);
void reservation_server();

#endif
