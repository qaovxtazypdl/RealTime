#include "kernel/syscall.h"
#include "common/constants.h"
#include "common/algs.h"
#include "tasks/name_server.h"
#include "bwio/bwio.h"

/*
valid replies are 0 to NUM_TASKS_MAX. Reserve others for error.
0 Success.
-1 The nameserver task id inside the wrapper is invalid.
*/
int RegisterAs(char *name) {
  int result = -1;
  char reply = (char) 0xff;
  struct NSRequest request;
  request.type = NS_REQ_REGISTER;
  strncpy(name, request.name, MAX_NAME_SIZE);

  result = Send(NS_TID, (char *) &request, sizeof(struct NSRequest), &reply, 1);

  if (result != 1 || reply == (char) 0xff) {
    return -1;
  } else {
    return 0;
  }
}

/*
valid replies are 0 to NUM_TASKS_MAX. Reserve others for error.
tid The task id of the registered task.
-1 The nameserver task id inside the wrapper is invalid.
*/
int WhoIs(char *name) {
  int result = -1;
  char reply = (char) -1;
  struct NSRequest request;
  request.type = NS_REQ_WHOIS;
  strncpy(name, request.name, MAX_NAME_SIZE);

  result = Send(NS_TID, (char *) &request, sizeof(struct NSRequest), &reply, 1);

  if (result != 1 || ((int) reply) >= NUM_TASKS_MAX) {
    return -1;
  } else {
    return (int) reply;
  }
}

#define NUM_NS_ENTRIES 1024
struct NSEntry {
  char name[MAX_NAME_SIZE];
  char tid;
};

void task__name_server() {
  int tid = -1;
  int retval = 1;
  char reply = (char) -1;
  struct NSRequest request;

  // allocate 1024 entries (32b each, total 32kb. stack has 200kb available per task at the time of writing)
  struct NSEntry entries[NUM_NS_ENTRIES];
  int numRegistrations = 0;
  int iterator = 0;
  int matchIndex;

  for (;;) {
    retval = Receive(&tid, (char *) &request, sizeof(struct NSRequest));
    if (retval < 0) {
      bwprintf(COM2, "Nameserver - Receive failed!\n\r", retval);
    }

    matchIndex = -1;
    for (iterator = 0; iterator < numRegistrations; iterator++) {
      if (strcmp(entries[iterator].name, request.name) == 0) {
        matchIndex = iterator;
        break;
      }
    }

    if (request.type == NS_REQ_REGISTER) {
      if (matchIndex < 0) {
        // new entry
        if (numRegistrations == NUM_NS_ENTRIES) {
          reply = (char) 0xff;
        } else {
          strncpy(request.name, entries[numRegistrations].name, MAX_NAME_SIZE);
          entries[numRegistrations].tid = tid;
          numRegistrations++;
          reply = 0;
        }
      } else {
        // replace
        entries[matchIndex].tid = tid;
        reply = 0;
      }
    } else if (request.type == NS_REQ_WHOIS) {
      if (matchIndex < 0) {
        // not found
        reply = (char) 0xff;
      } else {
        // return found
        reply = entries[matchIndex].tid;
      }
    } else {
      // error reply
      reply = (char) 0xff;
    }

    retval = Reply(tid, &reply, 1);
    if (retval < 0) {
      bwprintf(COM2, "Nameserver - Reply failed!\n\r", retval);
    }
  }
  Exit();
}




