#ifndef _NOTIFIER_H_
#define _NOTIFIER_H_
#include <common/event.h>

int create_notifier(int priority, enum event ev, void *payload);

#endif
