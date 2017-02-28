#ifndef _PROC_STATE_H_
#define _PROC_STATE_H_

/*  Represents a snapshot of the state of the CPU at a given point in time. Used as */
/*  glue between the C and assembly interrupt handlers.  */

/* DO NOT MODIFY WITHOUT UPDATING THE ASSEMBLY INTERRUPT HANDLERS */

typedef struct proc_state {
  int psr;
  int pc;
  int r0;
  int r1;
  int r2;
  int r3;
  int r4;
  int r5;
  int r6;
  int r7;
  int r8;
  int r9;
  int r10;
  int r11;
  int ip;
  int sp;
  int lr;
} proc_state_t;

#endif
