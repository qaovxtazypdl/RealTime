#include "kernel/context_switch.h"
#include "common/assert.h"
  .file "kernel/context_switch.S"
    .text
  .align  2
  .global kernel_entry_irq
  .type kernel_entry_irq, %function
kernel_entry_irq:
  @ args = 0, pretend = 0, frame = 0
  @ frame_needed = 1, uses_anonymous_args = 0

@ ======================== enter context switch =============================
@====================START=========================
  @ set to system mode
  msr cpsr_c, #0xdf

  @ save task registers
  stmfd sp!, {r0-r12, lr}

  @ acquire new sp into r4
  mov r4, sp

  @ set back to irq mode
  msr cpsr_c, #0xd2
@=====================END========================

  @ get lr_irq and spsr_irq
  sub r5, lr, #4
  mrs r7, spsr

  @ set back to system mode
  msr cpsr_c, #0xd3

  @load kernel registers
  ldmfd sp, {fp, sp, lr}

  @ get request number and save to request
  ldr r6, [fp, #-24]
  mov r1, #0x42
  str r1, [r6, #0]

  @ save pc, sp, spsr
  ldr r6, [fp, #-20]
  str r5, [r6, #0xc]
  str r4, [r6, #0x4]
  str r7, [r6, #0x8]
  @ save r0 to overwrite "return val field"
  str r0, [r6, #0x10]

  @ load the rest of the kernel registers
  ldmfd sp!, {r0-r10}
@ ======================== enter context switch =============================
@ ======================== enter context switch =============================

  sub sp, fp, #16
  ldmfd sp, {sl, fp, sp, pc}

  .size kernel_entry_irq, .-kernel_entry_irq
  .text
  .align  2
  .global kernel_entry
  .type kernel_entry, %function
kernel_entry:
  @ args = 0, pretend = 0, frame = 0
  @ frame_needed = 1, uses_anonymous_args = 0

@ ======================== enter context switch =============================
@ ======================== enter context switch =============================
@====================START=========================
  @ set to system mode
  msr cpsr_c, #0xdf

  @ save task registers
  stmfd sp!, {r0-r12, lr}

  @ acquire new sp into r4
  mov r4, sp

  @ set back to supervisor mode
  msr cpsr_c, #0xd3
@=====================END========================

  @ acquire lr into r5 (pc of user)
  mov r5, lr

  @load kernel registers
  ldmfd sp, {fp, sp, lr}

  @ get request number and save to request
  ldr r6, [fp, #-24]
  @ store args
  str r0, [r6, #0x4]
  str r1, [r6, #0x8]
  str r2, [r6, #0xc]
  str r3, [r6, #0x10]
  @ store request id
  ldr r0, [r5, #-4]
  and r0, r0, #0xffffff
  str r0, [r6, #0]
  @ store old user sp in case extra args are necessary
  str r4, [r6, #0x14]

  @ load up task descriptor
  ldr r6, [fp, #-20]

  @ save lr_svc/pc from lr_svc
  str r5, [r6, #0xc]

  @save sp
  str r4, [r6, #0x4]

  @ acquire spsr
  mrs r1, spsr
  @ save spsr
  str r1, [r6, #0x8]

  @ load the rest of the kernel registers
  ldmfd sp!, {r0-r10}
@ ======================== enter context switch =============================
@ ======================== enter context switch =============================

  sub sp, fp, #16
  ldmfd sp, {sl, fp, sp, pc}

  .size kernel_entry, .-kernel_entry
  .section  .rodata
  .align  2
.LC0:
  .ascii  "Goodbye kernel.\012\015\000"
  .align  2
.LC1:
  .ascii  "Hello again, kernel.\012\015\000"
  .align  2
  .global kernel_exit
  .type kernel_exit, %function
kernel_exit:
  @ args = 0, pretend = 0, frame = 0
  @ frame_needed = 1, uses_anonymous_args = 0
  mov ip, sp
  stmfd sp!, {sl, fp, ip, lr, pc}
  sub fp, ip, #4
  sub sp, sp, #8
.L5:
  add sl, pc, sl

  @ store locals
  str r0, [fp, #-20]
  str r1, [fp, #-24]

@ ======================== exit context switch ===========================
@ ======================== exit context switch ===========================
  @ save kernel registers
  stmfd sp!, {r0-r10}
  mov ip, sp
  stmfd sp!, {fp, ip, lr}

  @ load TD
  ldr r3, [fp, #-20]

  @ load saved task psr into spsr_svc
  ldr r2, [r3, #0x8]
  msr spsr, r2

  @ load pc into lr_svc
  ldr lr, [r3, #0xc]

  @ temporailiy save return val
  ldr r0, [r3, #0x10]
  stmfd sp!, {r0}

@========================START====================
  @ set to system mode
  msr cpsr_c, #0xdf

  @ load task regs
  ldr sp, [r3, #0x4]
  ldmfd sp!, {r0-r12, lr}

  @ set back to supervisor mode
  msr cpsr_c, #0xd3
@=======================END======================

  @ load return val from saved
  ldmfd sp!, {r0}

  @ CONTEXT SWITCH - LET IT GOOOOOO
  movs pc, lr

  @ assert.
  mov r0, #0
  bl ASSERT

@ ======================== exit context switch ===========================
@ ======================== exit context switch ===========================

.L7:
  .align  2
.L6:
  .word _GLOBAL_OFFSET_TABLE_-(.L5+8)
  .word .LC0(GOTOFF)
  .word .LC1(GOTOFF)
  .size kernel_exit, .-kernel_exit
  .ident  "GCC: (GNU) 4.0.2"
