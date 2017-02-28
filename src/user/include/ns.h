#ifndef _NS_H_
#define _NS_H_

#define MAX_NAME_LEN 256


void init_name_server();
int register_as(char *name);
int who_is(char *name);

#endif
