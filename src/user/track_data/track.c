#include <gen/track_data.h>
#include <track.h>
#include <sensors.h>

struct track_node g_track[TRACK_MAX];
struct track_node *g_sensors[NUM_SENSORS];
int g_is_track_a;
