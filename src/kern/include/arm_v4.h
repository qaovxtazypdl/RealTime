#ifndef _ARM_V4_H_
#define _ARM_V4_H_
/* Useful macros for dealing with arm asm. */

/* Store the opcode associated with the given instruction in the provided variable */
#define GET_OPCODE(dst, inst) \
asm volatile("ldr %0, [pc]\n \
b 1f\n" \
inst \
"\n1:" : "=r" (dst));

/* Store the opcode associated with the given instruction at the address associated with the given pointer */
#define STORE_OPCODE(ptr, inst) \
asm volatile("ldr r1, [pc]\n \
b 1f\n" \
inst \
"\n1: \n\t\
    mov r0, %0\n \
    str r1, [r0]" \
: : "r" (ptr) : "r0", "r1");

#define FETCH_REGISTER(dst, reg) \
  asm volatile("mov %0, "#reg"\n\t" : "=r" (dst) : : "sp", "r3");

#endif
