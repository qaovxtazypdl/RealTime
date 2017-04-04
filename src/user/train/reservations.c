#include <reservations.h>
#include <common/syscall.h>
#include <gen/track_data.h>
#include <track.h>
#include <train.h>
#include <console.h>
#include <io.h>

static int reservation_tid = -1;

struct reservation_request {
  struct track_node *track;
  int is_reserving;
};

int reserve_track(struct track_node *track) {
  struct reservation_request request;
  request.track = track;
  request.is_reserving = 1;
  int repl = 0;
  send(reservation_tid, &request, sizeof(struct reservation_request), &repl, 4);

  return repl;
}

int free_track(struct track_node *track) {
  struct reservation_request request;
  request.track = track;
  request.is_reserving = 0;
  int repl = 0;
  send(reservation_tid, &request, sizeof(struct reservation_request), &repl, 4);

  return repl;
}

void reservation_server() {
  reservation_tid = my_tid();

  int is_reserved[TRACK_MAX] = {0};
  // assume at most two clients for now - replace with RB if time allows
  int saved_reservation_requests[TRACK_MAX] = {0};

  struct reservation_request request;
  int repl = 0;
  int tid;
  while (1) {
    receive(&tid, &request, sizeof(struct reservation_request));
    if (request.is_reserving) {
      if (is_reserved[request.track - g_track]) {
        // already reserved - save reservation for later
        saved_reservation_requests[request.track - g_track] = tid;
        repl = is_reserved[request.track - g_track] == tid ? 2 : 0;
      } else {
        // repl immediately
        is_reserved[request.track - g_track] = tid;
        is_reserved[request.track->reverse - g_track] = tid;
        repl = 1;
      }
    } else {
      if (is_reserved[request.track - g_track] == tid) {
        // unreserve
        is_reserved[request.track - g_track] = 0;
        is_reserved[request.track->reverse - g_track] = 0;
        repl = 1;
      } else {
        // wtf
        repl = 0;
      }
    }
    reply(tid, &repl, 4);

  }
  exit();
}
