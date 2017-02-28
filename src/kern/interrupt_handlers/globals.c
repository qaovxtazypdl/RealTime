#include <kern.h>
#include <task.h>

/* definition of global kernel variables */
int kern_cts1 = 2;
int kern_cts2 = 2;
int kern_ticks = 0;
struct task *kern_current_task = NULL;
