#include "kernel/syscall.h"
  .file "kernel/syscall.S"
  .text
  .align  2
  .global Create
  .type Create, %function
Create:
  @ args = 0, pretend = 0, frame = 8
  @ frame_needed = 1, uses_anonymous_args = 0
  mov ip, sp
  stmfd sp!, {fp, ip, lr, pc}
  sub fp, ip, #4
  swi #0x1
  ldmfd sp, {fp, sp, pc}
  .size Create, .-Create
  .align  2
  .global MyTid
  .type MyTid, %function
MyTid:
  @ args = 0, pretend = 0, frame = 0
  @ frame_needed = 1, uses_anonymous_args = 0
  mov ip, sp
  stmfd sp!, {fp, ip, lr, pc}
  sub fp, ip, #4
  swi #0x10
  ldmfd sp, {fp, sp, pc}
  .size MyTid, .-MyTid
  .align  2
  .global MyParentTid
  .type MyParentTid, %function
MyParentTid:
  @ args = 0, pretend = 0, frame = 0
  @ frame_needed = 1, uses_anonymous_args = 0
  mov ip, sp
  stmfd sp!, {fp, ip, lr, pc}
  sub fp, ip, #4
  swi #0x11
  ldmfd sp, {fp, sp, pc}
  .size MyParentTid, .-MyParentTid
  .align  2
  .global Pass
  .type Pass, %function
Pass:
  @ args = 0, pretend = 0, frame = 0
  @ frame_needed = 1, uses_anonymous_args = 0
  mov ip, sp
  stmfd sp!, {fp, ip, lr, pc}
  sub fp, ip, #4
  swi #0xfe
  ldmfd sp, {fp, sp, pc}
  .size Pass, .-Pass
  .align  2
  .global Exit
  .type Exit, %function
Exit:
  @ args = 0, pretend = 0, frame = 0
  @ frame_needed = 1, uses_anonymous_args = 0
  mov ip, sp
  stmfd sp!, {fp, ip, lr, pc}
  sub fp, ip, #4
  swi #0xff
  ldmfd sp, {fp, sp, pc}
  .size Exit, .-Exit
  .align  2
  .global Send
  .type Send, %function
Send:
  @ args = 4, pretend = 0, frame = 16
  @ frame_needed = 1, uses_anonymous_args = 0
  mov ip, sp
  stmfd sp!, {fp, ip, lr, pc}
  sub fp, ip, #4
  swi #0x20
  ldmfd sp, {fp, sp, pc}
  .size Send, .-Send
  .align  2
  .global Receive
  .type Receive, %function
Receive:
  @ args = 0, pretend = 0, frame = 12
  @ frame_needed = 1, uses_anonymous_args = 0
  mov ip, sp
  stmfd sp!, {fp, ip, lr, pc}
  sub fp, ip, #4
  swi #0x21
  ldmfd sp, {fp, sp, pc}
  .size Receive, .-Receive
  .align  2
  .global Reply
  .type Reply, %function
Reply:
  @ args = 0, pretend = 0, frame = 12
  @ frame_needed = 1, uses_anonymous_args = 0
  mov ip, sp
  stmfd sp!, {fp, ip, lr, pc}
  sub fp, ip, #4
  swi #0x22
  ldmfd sp, {fp, sp, pc}
  .size Reply, .-Reply
  .align  2
  .global AwaitEvent
  .type AwaitEvent, %function
AwaitEvent:
  @ args = 0, pretend = 0, frame = 4
  @ frame_needed = 1, uses_anonymous_args = 0
  mov ip, sp
  stmfd sp!, {fp, ip, lr, pc}
  sub fp, ip, #4
  swi #0x30
  ldmfd sp, {fp, sp, pc}
  .size AwaitEvent, .-AwaitEvent
  .align  2
  .global MyProfilerTime
  .type MyProfilerTime, %function
MyProfilerTime:
  @ args = 0, pretend = 0, frame = 8
  @ frame_needed = 1, uses_anonymous_args = 0
  mov ip, sp
  stmfd sp!, {fp, ip, lr, pc}
  sub fp, ip, #4
  swi #0xf0
  ldmfd sp, {fp, sp, pc}
  .size MyProfilerTime, .-MyProfilerTime
  .align  2
  .global Shutdown
  .type Shutdown, %function
Shutdown:
  @ args = 0, pretend = 0, frame = 0
  @ frame_needed = 1, uses_anonymous_args = 0
  mov ip, sp
  stmfd sp!, {fp, ip, lr, pc}
  sub fp, ip, #4
  swi #0xfc
  ldmfd sp, {fp, sp, pc}
  .size Shutdown, .-Shutdown
  .ident  "GCC: (GNU) 4.0.2"
