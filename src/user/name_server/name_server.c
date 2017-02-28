#include <ns.h>
#include <assert.h>
#include <common/string.h>
#include <common/ds/queue.h>
#include <common/syscall.h>

#define MAX_TASKS 100 

static int init = 0;
static int ns_td = -1;

static void name_server();

struct ns_msg {
  enum ns_op {
    NS_OP_REGISTER,
    NS_OP_LOOKUP
  } op;
  char name[MAX_NAME_LEN];
  int td;
};

void init_name_server() {
  assert(init == 0, "init_name_server() must be called exactly once");
  ns_td = create(0, name_server);
  init++;
}

int register_as(char *name) {
  assert(init == 1, "init_name_server() must be called exactly once");

  int rc;
  struct ns_msg msg;

  msg.op = NS_OP_REGISTER;
  strcpy(msg.name, name);
  send(ns_td, &msg, sizeof(msg), NULL, 0);

  return (rc < 0) ? rc : 0;
}

int who_is(char *name) { /* -1 indicates failure */
  assert(init == 1, "init_name_server() must be called exactly once");

  int r, rc;
  struct ns_msg msg;

  msg.op = NS_OP_LOOKUP;
  strcpy(msg.name, name);
  rc = send(ns_td, (char*)&msg, sizeof(msg), (char*)&r, sizeof(r));

  return (rc < 0) ? rc : r;
}


/* This blocked if a task with the given name does not exist yet. */
static void name_server() {
  int td, i, response;
  struct ns_msg msg;

  static queue_t blocked_whois_requests;
  struct ns_msg blocked_whois_requests_mem[MAX_TASKS];
  static char name_map[MAX_TASKS][MAX_NAME_LEN];

  queue_init(&blocked_whois_requests,
      blocked_whois_requests_mem,
      sizeof(blocked_whois_requests_mem),
      sizeof(blocked_whois_requests_mem[0]));


  while(1) {
    /* Expects string to contain terminating null */
    receive(&td, &msg, sizeof(msg)); 
    msg.td = td;

    if(msg.op == NS_OP_REGISTER) {
      strcpy(name_map[td], msg.name);
      reply(td, NULL, 0);

      /* FIXME this is madness */
      struct ns_msg start, it;
      if(!queue_consume(&blocked_whois_requests, &it)) {
        start = it;
        do {
          if(streq(msg.name, it.name))
            queue_add(&blocked_whois_requests, &it);
          else {
            reply(it.td, &msg.td, sizeof(msg.td));
          }
        }  while(!queue_consume(&blocked_whois_requests, &it) && (it.td != start.td));
      }
    } else {
      response = -1; /* Lookup failed */
      for (i = 0; i < MAX_NAME_LEN; i++) {
        if(!streq(name_map[i], msg.name)) {
          response = i;
        }
      }

      if(response == -1)
        queue_add(&blocked_whois_requests, &msg);
      else {
        reply(td, &response, sizeof(response));
      }
    }
  }
}
