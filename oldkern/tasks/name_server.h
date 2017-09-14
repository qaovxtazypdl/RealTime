#ifndef _TASKS__NAME_SERVER_H_
#define _TASKS__NAME_SERVER_H_

#define MAX_NAME_SIZE 31
#define NS_TID 1
#define NS_REQ_REGISTER 0
#define NS_REQ_WHOIS 1

struct NSRequest {
  char type;
  char name[MAX_NAME_SIZE];
};

void task__name_server();

/*
0 Success.
-1 The nameserver task id inside the wrapper is invalid.
*/
int RegisterAs(char *name);

/*
tid The task id of the registered task.
-1 The nameserver task id inside the wrapper is invalid.
*/
int WhoIs(char *name);

#endif
