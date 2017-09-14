#ifndef _COMMON__CONSTANTS_H_
#define _COMMON__CONSTANTS_H_

typedef char *va_list;

#define __va_argsiz(t)  \
    (((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(ap, pN) ((ap) = ((va_list) __builtin_next_arg(pN)))

#define va_end(ap)  ((void)0)

#define va_arg(ap, t) \
     (((ap) = (ap) + __va_argsiz(t)), *((t*) (void*) ((ap) - __va_argsiz(t))))

#define NULL ((void *) 0)
#define OFF 0
#define ON 1
#define NO 0
#define YES 1
#define TRUE 1
#define FALSE 0

#define NUM_TASKS_MAX 128

#define EVENT_TIMER_TICK 51
#define EVENT_UART1_GENERAL 52
#define EVENT_UART2_GENERAL 54

#define EVENT_UART1_RCV_RDY 1
#define EVENT_UART1_XMIT_RDY 2
#define EVENT_UART1_MODEM 3
#define EVENT_UART1_RCV_TIMEOUT 4

#define EVENT_UART2_RCV_RDY 5
#define EVENT_UART2_XMIT_RDY 6
#define EVENT_UART2_MODEM 7
#define EVENT_UART2_RCV_TIMEOUT 8


// trains
#define NUM_TRAINS_MAX 128
#define NUM_SWITCHES 22

#endif
