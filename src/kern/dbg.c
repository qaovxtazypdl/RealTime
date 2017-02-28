#include <common/bwio.h>
#include <common/syscall.h>
#include <dbg.h>

/* Do not call this directly, use the PRINT_REG macro */

void dbg_print_reg() {
  int val;

  dbg_noprefix("Register dump: ");
  dbg_noprefix("##################################################################################\n\r");
  asm("ldr r0, [fp, #8]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("r0: 0x%x", val);

  asm("ldr r0, [fp, #12]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("r1: 0x%x", val);

  asm("ldr r0, [fp, #16]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("r2: 0x%x", val);

  asm("ldr r0, [fp, #20]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("r3: 0x%x", val);

  asm("ldr r0, [fp, #24]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("r4: 0x%x", val);

  asm("ldr r0, [fp, #28]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("r5: 0x%x", val);

  asm("ldr r0, [fp, #32]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("r6: 0x%x", val);

  asm("ldr r0, [fp, #36]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("r7: 0x%x", val);

  asm("ldr r0, [fp, #40]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("r8: 0x%x", val);

  asm("ldr r0, [fp, #44]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("r9: 0x%x", val);

  asm("ldr r0, [fp, #48]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("r10: 0x%x", val);

  asm("ldr r0, [fp, #52]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("r11: 0x%x", val);

  asm("ldr r0, [fp, #56]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("r12: 0x%x", val);

  asm("ldr r0, [fp, #60]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("sp: 0x%x", val);

  asm("ldr r0, [fp, #64]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("lr: 0x%x", val);

  /* The stmfd instruction causes the pc to be incremented before it is stored, thus we subtract 4 to obtain */
  /* its value at the point of interest. (FIXME*) */
  asm("ldr r0, [fp, #68]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("pc: 0x%x", val - 4);

  asm("ldr r0, [fp, #4]\n\r\t"
    "mov %0, r0\r\t"
    : "=r" (val));
  dbg_noprefix("CPSR: 0x%x", val);

  dbg_noprefix("##################################################################################\n\r");
}

void dbg_print_bytes(char *start, int len) {
  int i;
  for (i = 0; i < len; i++)
    dbg_noprefixnnl("%x ", start[i]);

  dbg_noprefix("");
}

void dbg_trigger_timer3_int() {
  *((int*)(VIC2_BASE + VIC_SOFTINT_OFFSET)) = 0x80000;
}

